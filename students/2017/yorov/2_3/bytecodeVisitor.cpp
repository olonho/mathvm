#include "bytecodeVisitor.h"
#include <iostream>
#include <cmath>
#include <unordered_map>
#include <dlfcn.h>

namespace mathvm {

    double BytecodeVisitor::epsilon = 1e-11;

    Status* BytecodeVisitor::start(AstFunction* astFunction) {
        BytecodeFunction* bytecodeFunction = new BytecodeFunction(astFunction);
        _code->addFunction(bytecodeFunction);
        try {
            visitAstFunction(astFunction);
        } catch(std::logic_error& e) {
            return Status::Error(e.what());
        }
        return Status::Ok();
    }

    void BytecodeVisitor::visitAstFunction(AstFunction* astFunction) {
        Scope* scope = astFunction->scope();
        BytecodeFunction* func = static_cast<BytecodeFunction*>(_code->functionByName(astFunction->name()));
        utils::Context* innerContext = new utils::Context(func, _context);
        Scope::VarIterator it(scope);
        while (it.hasNext()) {
            innerContext->addVar(it.next()->name());
        }
        _context = innerContext;

        for (uint32_t i = 0; i < astFunction->parametersNumber(); ++i) {
            const std::string& varName = astFunction->parameterName(i);
            AstVar* var = astFunction->scope()->lookupVariable(varName, false);
            storeTOS(var);
        }

        astFunction->node()->visit(this);
        func->setScopeId(_context->id());
        func->setLocalsNumber(_context->localsCount());

        _context = _context->parent();
        delete innerContext;
    }

    void BytecodeVisitor::visitBinaryOpNode(BinaryOpNode* node) {
        TokenKind kind = node->kind();
        switch (kind) {
            case tADD:
            case tSUB:
            case tMUL:
            case tDIV:
            case tMOD:
                visitArithmeticOp(node);
                break;
            case tOR:
            case tAND:
                visitLogicalOp(node);
                break;
            case tAOR:
            case tAAND:
            case tAXOR:
                visitBitwiseOp(node);
                break;
            case tEQ:
            case tNEQ:
            case tGT:
            case tGE:
            case tLT:
            case tLE:
                visitComparisonOp(node);
                break;
            default:
                throw std::logic_error(std::string("Unknown operator: ") + tokenOp(kind));
        }
    }

    void BytecodeVisitor::visitArithmeticOp(BinaryOpNode* node) {
        TokenKind kind = node->kind();
        node->left()->visit(this);
        VarType left = _TOSType;
        node->right()->visit(this);
        VarType right = _TOSType;

        VarType resultType = utils::resultType(left, right);
        if (resultType == VT_INVALID) {
            throw std::logic_error(std::string("wrong operands in operation: ")
                                    + typeToName(left) + tokenOp(kind) + typeToName(right));
        }

        Instruction insn = utils::arithmeticInsn(resultType, kind);
        if (insn == BC_INVALID) {
            throw std::logic_error(std::string("unsupported operation: ")
                                    + typeToName(left) + tokenOp(kind) + typeToName(right));
        }

        Bytecode* bytecode = _context->function()->bytecode();
        if (right != resultType) {
            convert(right, resultType);
        }

        if (left != resultType) {
            bytecode->addInsn(BC_SWAP);
            convert(left, resultType);
            bytecode->addInsn(BC_SWAP);
        }

        bytecode->addInsn(insn);
        _TOSType = resultType;
    }

    void BytecodeVisitor::visitLogicalOp(BinaryOpNode* node) {
        TokenKind kind = node->kind();
        Bytecode* bytecode = _context->function()->bytecode();

        Label setTrue(bytecode);
        Label setFalse(bytecode);

        if (kind == tAND) {
            bytecode->addInsn(BC_ILOAD0);
            node->left()->visit(this);
            convert(_TOSType, VT_INT);
            _TOSType = VT_INT;
            bytecode->addBranch(BC_IFICMPE, setFalse);
            bytecode->addInsn(BC_POP);
            bytecode->addInsn(BC_POP);

            bytecode->addInsn(BC_ILOAD0);
            node->right()->visit(this);
            convert(_TOSType, VT_INT);
            _TOSType = VT_INT;
            bytecode->addBranch(BC_IFICMPE, setFalse);
            bytecode->addInsn(BC_POP);
            bytecode->addInsn(BC_POP);

            bytecode->addInsn(BC_ILOAD1);
            bytecode->addBranch(BC_JA, setTrue);

            bytecode->bind(setFalse);

            bytecode->addInsn(BC_POP);
            bytecode->addInsn(BC_POP);

            bytecode->addInsn(BC_ILOAD0);
            bytecode->bind(setTrue);
            _TOSType = VT_INT;
            return;
        }

        // kind == OR
        bytecode->addInsn(BC_ILOAD0);
        node->left()->visit(this);
        convert(_TOSType, VT_INT);
        _TOSType = VT_INT;
        bytecode->addBranch(BC_IFICMPNE, setTrue);
        bytecode->addInsn(BC_POP);
        bytecode->addInsn(BC_POP);

        bytecode->addInsn(BC_ILOAD0);
        node->right()->visit(this);
        convert(_TOSType, VT_INT);
        _TOSType = VT_INT;
        bytecode->addBranch(BC_IFICMPNE, setTrue);
        bytecode->addInsn(BC_POP);
        bytecode->addInsn(BC_POP);

        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_JA, setFalse);

        bytecode->bind(setTrue);
        bytecode->addInsn(BC_POP);
        bytecode->addInsn(BC_POP);

        bytecode->addInsn(BC_ILOAD1);
        bytecode->bind(setFalse);
        _TOSType = VT_INT;
    }

    void BytecodeVisitor::visitBitwiseOp(BinaryOpNode* node) {
        node->left()->visit(this);
        VarType left = _TOSType;
        convert(left, VT_INT);
        _TOSType = VT_INT;
        node->right()->visit(this);
        VarType right = _TOSType;
        convert(right, VT_INT);
        _TOSType = VT_INT;

        TokenKind kind = node->kind();
        Instruction insn = utils::bitwiseInsn(kind);
        if (insn == BC_INVALID) {
            throw std::logic_error(std::string("unsupported bitwise operation: ") + tokenOp(kind));
        }

        Bytecode* bytecode = _context->function()->bytecode();
        bytecode->addInsn(insn);
        _TOSType = VT_INT;
    }

    void BytecodeVisitor::visitComparisonOp(BinaryOpNode* node) {
        TokenKind kind = node->kind();
        node->left()->visit(this);
        VarType left = _TOSType;
        node->right()->visit(this);
        VarType right = _TOSType;

        VarType resultType = utils::resultType(left, right);
        if (resultType == VT_INVALID) {
            throw std::logic_error(std::string("wrong operands in comparison: ")
                                    + typeToName(left) + tokenOp(kind) + typeToName(right));
        }

        Bytecode* bytecode = _context->function()->bytecode();

        if (right != resultType) {
            convert(right, resultType);
        }

        if (left != resultType) {
            bytecode->addInsn(BC_SWAP);
            convert(left, resultType);
            bytecode->addInsn(BC_SWAP);
        }

        // maybe here swap before
        // resultType either VT_INT or VT_DOUBLE (see utils::resultType)
        if (resultType == VT_DOUBLE) {
            bytecode->addInsn(BC_DCMP);
        } else {
            bytecode->addInsn(BC_ICMP);
        }
        Instruction insn = utils::comparisonInsn(kind);
        if (insn == BC_INVALID) {
            throw std::logic_error(std::string("unsupported comparison operation: ")
                                    + typeToName(left) + tokenOp(kind) + typeToName(right));
        }

        Label setTrue(bytecode);
        Label setFalse(bytecode);

        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(insn, setTrue);
        bytecode->addInsn(BC_POP);
        bytecode->addInsn(BC_POP);
        bytecode->addInsn(BC_POP);
        bytecode->addInsn(BC_POP);

        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_JA, setFalse);

        bytecode->bind(setTrue);
        bytecode->addInsn(BC_POP);
        bytecode->addInsn(BC_POP);
        bytecode->addInsn(BC_POP);
        bytecode->addInsn(BC_POP);

        bytecode->addInsn(BC_ILOAD1);

        bytecode->bind(setFalse);
        _TOSType = VT_INT;
    }

    void BytecodeVisitor::visitUnaryOpNode(UnaryOpNode* node) {
        node->operand()->visit(this);
        VarType operandType = _TOSType;

        if (operandType != VT_INT && operandType != VT_DOUBLE && operandType != VT_STRING) {
            throw std::logic_error(std::string("unexpected operand type ") + typeToName(operandType));
        }

        TokenKind kind = node->kind();
        Bytecode* bytecode = _context->function()->bytecode();
        switch (kind) {
            case tNOT: {
                convert(operandType, VT_INT);
                _TOSType = VT_INT;
                Label setTrue(bytecode);
                Label setFalse(bytecode);

                bytecode->addInsn(BC_ILOAD0);
                bytecode->addBranch(BC_IFICMPE, setTrue);
                bytecode->addInsn(BC_POP);
                bytecode->addInsn(BC_POP);

                bytecode->addInsn(BC_ILOAD0);
                bytecode->addBranch(BC_JA, setFalse);

                bytecode->bind(setTrue);
                bytecode->addInsn(BC_POP);
                bytecode->addInsn(BC_POP);

                bytecode->addInsn(BC_ILOAD1);
                bytecode->bind(setFalse);
                break;
            }
            case tSUB: {
                if (operandType == VT_DOUBLE) {
                    bytecode->addInsn(BC_DNEG);
                } else {
                    convert(operandType, VT_INT);
                    bytecode->addInsn(BC_INEG);
                }
                break;
            }
            default:
                throw std::logic_error(std::string("unexpected unary operator ") + tokenOp(kind));
        }
    }

    void BytecodeVisitor::visitStringLiteralNode(StringLiteralNode* node) {
        const std::string& literal = node->literal();
        Bytecode* bytecode = _context->function()->bytecode();
        if (literal.empty()) {
            bytecode->addInsn(BC_SLOAD0);
        } else {
            uint16_t literalId = _code->makeStringConstant(literal);
            bytecode->addInsn(BC_SLOAD);
            bytecode->addUInt16(literalId);
        }
        _TOSType = VT_STRING;
    }

    void BytecodeVisitor::visitIntLiteralNode(IntLiteralNode* node) {
        int64_t literal = node->literal();
        Bytecode* bytecode = _context->function()->bytecode();
        switch (literal) {
            case 0:
                bytecode->addInsn(BC_ILOAD0);
                break;
            case 1:
                bytecode->addInsn(BC_ILOAD1);
                break;
            case -1:
                bytecode->addInsn(BC_ILOADM1);
                break;
            default:
                bytecode->addInsn(BC_ILOAD);
                bytecode->addInt64(literal);
                break;
        }
        _TOSType = VT_INT;
    }

    void BytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
        double literal = node->literal();
        Bytecode* bytecode = _context->function()->bytecode();
        _TOSType = VT_DOUBLE;
        if (fabs(literal) < epsilon) {
            bytecode->addInsn(BC_DLOAD0);
            return;
        }
        if (fabs(literal - 1.) < epsilon) {
            bytecode->addInsn(BC_DLOAD1);
            return;
        }

        if (fabs(literal + 1.) < epsilon) {
            bytecode->addInsn(BC_DLOADM1);
            return;
        }

        bytecode->addInsn(BC_DLOAD);
        bytecode->addDouble(literal);
    }

    void BytecodeVisitor::visitLoadNode(LoadNode* node) {
        loadToTOS(node->var());
    }

    void BytecodeVisitor::visitStoreNode(StoreNode* node) {
        node->value()->visit(this);
        TokenKind kind = node->op();
        if (kind != tINCRSET && kind != tDECRSET && kind != tASSIGN) {
            throw std::logic_error(std::string("undefined store operation ") + tokenOp(kind));
        }

        Bytecode* bytecode = _context->function()->bytecode();
        std::unordered_map<TokenKind, TokenKind> temp;
        temp.insert(std::make_pair(tINCRSET, tADD));
        temp.insert(std::make_pair(tDECRSET, tSUB));
        if (kind == tINCRSET || kind == tDECRSET) {
            VarType varType = node->var()->type();
            convert(_TOSType, varType);
            _TOSType = varType;
            loadToTOS(node->var());
            Instruction insn = utils::arithmeticInsn(varType, temp[kind]);
            if (insn == BC_INVALID) {
                throw std::logic_error(std::string("undefined operation ")
                                        + tokenOp(temp[kind]) + std::string(" for ") + typeToName(_TOSType));
            }
            bytecode->addInsn(BC_SWAP);
            bytecode->addInsn(insn);
        }
        storeTOS(node->var());
    }

    void BytecodeVisitor::visitBlockNode(BlockNode* node) {
        visitScope(node->scope());
        node->visitChildren(this);
    }

    void BytecodeVisitor::visitNativeCallNode(NativeCallNode* node) {
        void* code = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
        if (code == nullptr) {
            throw std::logic_error("Native function not found");
        }
        uint16_t nativeId = _code->makeNativeFunction(node->nativeName(), node->nativeSignature(), code);

        Bytecode* bytecode = _context->function()->bytecode();
        bytecode->addInsn(BC_CALLNATIVE);
        bytecode->addUInt16(nativeId);

        VarType returnType = node->nativeSignature()[0].first;
        if (returnType != VT_VOID && returnType != VT_INVALID) {
            _TOSType = returnType;
        }
    }


    void BytecodeVisitor::visitCallNode(CallNode* node) {
        const std::string& name = node->name();
        TranslatedFunction* trFunc = _code->functionByName(name);
        if (trFunc == nullptr) {
            throw std::logic_error(std::string("function ") + name + std::string(" not found"));
        }

        for (int i = trFunc->parametersNumber() - 1; i >= 0; --i) {
            // function may has only uint16_t::max() params, therefore this cast are safe
            uint16_t index = static_cast<uint16_t>(i);
            node->parameterAt(index)->visit(this);
            convert(_TOSType, trFunc->parameterType(index));
            _TOSType = trFunc->parameterType(index);
        }

        Bytecode* bytecode = _context->function()->bytecode();
        bytecode->addInsn(BC_CALL);
        bytecode->addUInt16(trFunc->id());

        VarType returnType = trFunc->returnType();
        if (returnType != VT_INVALID && returnType != VT_VOID) {
            _TOSType = returnType;
        }
    }

    void BytecodeVisitor::visitForNode(ForNode* node) {
        const AstVar* var = node->var();
        if (var->type() != VT_INT) {
            throw std::logic_error(std::string("for variable are not VT_INT"));
        }
        BinaryOpNode* opNode = node->inExpr()->asBinaryOpNode();
        if (opNode->kind() != tRANGE) {
            throw std::logic_error(std::string("unsupported operation in for ") + tokenOp(opNode->kind()));
        }
        Bytecode* bytecode = _context->function()->bytecode();
        opNode->left()->visit(this);
        convert(_TOSType, VT_INT);
        _TOSType = VT_INT;
        storeTOS(var);

        opNode->right()->visit(this);
        convert(_TOSType, VT_INT);
        _TOSType = VT_INT;

        Label beginLabel(bytecode);
        Label endLabel(bytecode);
        bytecode->bind(beginLabel);

        loadToTOS(var);
        bytecode->addBranch(BC_IFICMPL, endLabel);
        bytecode->addInsn(BC_POP);

        node->body()->visit(this);

        bytecode->addInsn(BC_ILOAD1);
        loadToTOS(var);
        bytecode->addInsn(BC_IADD);
        storeTOS(var);

        bytecode->addBranch(BC_JA, beginLabel);
        bytecode->bind(endLabel);
        bytecode->addInsn(BC_POP);
        bytecode->addInsn(BC_POP);
    }

    void BytecodeVisitor::visitWhileNode(WhileNode* node) {
        Bytecode* bytecode = _context->function()->bytecode();

        Label beginLabel(bytecode);
        Label endLabel(bytecode);

        bytecode->bind(beginLabel);
        node->whileExpr()->visit(this);
        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_IFICMPE, endLabel);
        bytecode->addInsn(BC_POP);
        bytecode->addInsn(BC_POP);

        node->loopBlock()->visit(this);
        bytecode->addBranch(BC_JA, beginLabel);
        bytecode->bind(endLabel);
        bytecode->addInsn(BC_POP);
        bytecode->addInsn(BC_POP);
    }

    void BytecodeVisitor::visitIfNode(IfNode* node) {
        Bytecode* bytecode = _context->function()->bytecode();
        bool hasElseBlock = node->elseBlock() != nullptr;

        Label elseLabel(bytecode);
        Label thenLabel(bytecode);

        node->ifExpr()->visit(this);
        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_IFICMPE, elseLabel);
        bytecode->addInsn(BC_POP);
        bytecode->addInsn(BC_POP);

        node->thenBlock()->visit(this);
        bytecode->addBranch(BC_JA, thenLabel);

        bytecode->bind(elseLabel);
        bytecode->addInsn(BC_POP);
        bytecode->addInsn(BC_POP);
        if (hasElseBlock) {
            node->elseBlock()->visit(this);
        }

        bytecode->bind(thenLabel);
    }

    void BytecodeVisitor::visitReturnNode(ReturnNode* node) {
        VarType varType = _context->function()->returnType();
        if (node->returnExpr() != nullptr) {
            node->returnExpr()->visit(this);
            convert(_TOSType, varType);
            _TOSType = varType;
        }
        Bytecode* bytecode = _context->function()->bytecode();
        bytecode->addInsn(BC_RETURN);
        if (varType != VT_VOID && varType != VT_INVALID) {
            _TOSType = varType;
        }
    }

    void BytecodeVisitor::visitFunctionNode(FunctionNode* node) {
        node->body()->visit(this);
    }

    void BytecodeVisitor::visitPrintNode(PrintNode* node) {
        Bytecode* bytecode = _context->function()->bytecode();
        for (size_t i = 0; i < node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            switch (_TOSType) {
                case VT_INT:
                    bytecode->addInsn(BC_IPRINT);
                    break;
                case VT_DOUBLE:
                    bytecode->addInsn(BC_DPRINT);
                    break;
                case VT_STRING:
                    bytecode->addInsn(BC_SPRINT);
                    break;
                default:
                    throw std::logic_error(std::string("cannot print type ") + typeToName(_TOSType));
            }
        }
        _TOSType = VT_VOID;
    }

    void BytecodeVisitor::visitScope(Scope* scope) {
        Scope::VarIterator varIter{scope};
        while (varIter.hasNext()) {
            _context->addVar(varIter.next()->name());
        }

        Scope::FunctionIterator funcIter{scope};
        while (funcIter.hasNext()) {
            AstFunction* func = funcIter.next();
            TranslatedFunction* trFunc = _code->functionByName(func->name());
            if (trFunc != nullptr) {
                throw std::logic_error(std::string("one definition rule"));
            }
            trFunc = new BytecodeFunction(func);
            _code->addFunction(trFunc);
        }

        funcIter = Scope::FunctionIterator{scope};
        while (funcIter.hasNext()) {
            AstFunction* func = funcIter.next();
            visitAstFunction(func);
        }
    }

    void BytecodeVisitor::convert(VarType from, VarType to) {
        if (from == to) {
            return;
        }
        Bytecode* bytecode = _context->function()->bytecode();
        auto insns = utils::insnConvert(from, to);
        for (size_t i = 0; i < insns.size(); ++i) {
            if (insns[i] == BC_INVALID) {
                throw std::logic_error(std::string("cannot cast from: ")
                                        + typeToName(from) + std::string(" to ") + typeToName(to));
            }
            bytecode->addInsn(insns[i]);
        }
    }

    void BytecodeVisitor::loadToTOS(const AstVar* var) {
        auto locAndCtxId = _context->locAndCtxId(var->name());
        uint16_t localId = locAndCtxId.first;
        uint16_t contextId = locAndCtxId.second;
        VarType varType = var->type();

        uint16_t curCtxtId = _context->id();
        bool inLocalScope = curCtxtId == contextId;
        Instruction insn = inLocalScope
                               ? utils::localScopeLoadInsn(varType, localId)
                               : utils::outerScopeLoadInsn(varType);

        if (insn == BC_INVALID) {
            throw std::logic_error(std::string("cannot get load instruction for ") + var->name());
        }

        Bytecode* bytecode = _context->function()->bytecode();
        bytecode->addInsn(insn);
        if (!inLocalScope) {
           bytecode->addUInt16(contextId);
           bytecode->addUInt16(localId);
        } else if (localId > 3) {
           bytecode->addUInt16(localId);
        }
        _TOSType = varType;
    }

    void BytecodeVisitor::storeTOS(const AstVar* var) {
        auto locAndCtxId = _context->locAndCtxId(var->name());
        uint16_t localId = locAndCtxId.first;
        uint16_t contextId = locAndCtxId.second;
        VarType varType = var->type();

        uint16_t curCtxtId = _context->id();
        bool inLocalScope = curCtxtId == contextId;
        Instruction insn = inLocalScope
                               ? utils::localScopeStoreInsn(varType, localId)
                               : utils::outerScopeStoreInsn(varType);

        if (insn == BC_INVALID) {
           throw std::logic_error(std::string("cannot get store instruction for ") + var->name());
        }
        Bytecode* bytecode = _context->function()->bytecode();
        bytecode->addInsn(insn);
        if (!inLocalScope) {
            bytecode->addUInt16(contextId);
            bytecode->addUInt16(localId);
        } else if (localId > 3) {
            bytecode->addUInt16(localId);
        }

        _TOSType = VT_VOID;
    }

    BytecodeVisitor::BytecodeVisitor(Code* code)
        : _code(code)
        , _TOSType(VT_VOID)
    {}

    BytecodeVisitor::~BytecodeVisitor() {}
}
