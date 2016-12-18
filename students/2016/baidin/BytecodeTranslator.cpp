#include "BytecodeTranslator.h"
#include "ast.h"

namespace mathvm {

    ToBytecodeVisitor::~ToBytecodeVisitor() {}

    void ToBytecodeVisitor::visitForNode(ForNode *node) {
        if (!node->inExpr()->isBinaryOpNode() || node->inExpr()->asBinaryOpNode()->kind() != tRANGE) {
            throw TranslationException{"Not a range in for loop!"};
        }
        if (node->var()->type() != VT_INT) {
            throw TranslationException{"Var must be int!"};
        }

        BinaryOpNode *rangeNode = node->inExpr()->asBinaryOpNode();

        rangeNode->left()->visit(this);
        castToType(VT_INT);
        storeVarValue(node->var()->name(), node->var()->type());

        rangeNode->right()->visit(this);
        castToType(VT_INT);
        bytecode->addInsn(BC_STOREIVAR0);

        Label checkLabel = Label{bytecode};
        Label endLabel = Label{bytecode};

        bytecode->bind(checkLabel);


        loadVarValue(node->var()->name(), node->var()->type());
        bytecode->addInsn(BC_LOADIVAR0);
        bytecode->addBranch(BC_IFICMPG, endLabel);

        node->body()->visit(this);

        loadVarValue(node->var()->name(), node->var()->type());
        bytecode->addInsn(BC_ILOAD1);
        bytecode->addInsn(BC_IADD);
        storeVarValue(node->var()->name(), node->var()->type());
        bytecode->addBranch(BC_JA, checkLabel);
        bytecode->bind(endLabel);

        topType = VT_VOID;
    }

    void ToBytecodeVisitor::visitPrintNode(PrintNode *node) {
        for (uint32_t i = 0; i < node->operands(); ++i) {
            print(node->operandAt(i));
        }
    }

    void ToBytecodeVisitor::visitLoadNode(LoadNode *node) {
        loadVarValue(node->var()->name(), node->var()->type());
    }

    void ToBytecodeVisitor::visitIfNode(IfNode *node) {
        node->ifExpr()->visit(this);
        castToType(VT_INT);

        Label elseLabel = Label{bytecode};
        Label endLabel = Label{bytecode};

        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_IFICMPE, elseLabel);

        node->thenBlock()->visit(this);
        bytecode->addBranch(BC_JA, endLabel);

        bytecode->bind(elseLabel);
        if (node->elseBlock()) {
            node->elseBlock()->visit(this);
        }

        bytecode->bind(endLabel);

        topType = VT_VOID;
    }

    void ToBytecodeVisitor::visitCallNode(CallNode *node) {
        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            node->parameterAt(i)->visit(this);
            castToType(code->functionByName(node->name())->parameterType(i));
        }
        bytecode->addInsn(BC_CALL);
        bytecode->addUInt16(code->functionByName(node->name())->id());
        topType = code->functionByName(node->name())->returnType();
    }

    void ToBytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
        bytecode->addInsn(BC_DLOAD);
        bytecode->addDouble(node->literal());
        topType = VT_DOUBLE;
    }

    void ToBytecodeVisitor::visitStoreNode(StoreNode *node) {
        if (node->op() == tINCRSET || node->op() == tDECRSET) {
            loadVarValue(node->var()->name(), node->var()->type());
        }

        node->value()->visit(this);
        castToType(node->var()->type());

        if (node->op() == tINCRSET) {
            performArithmeticOperation(node->var()->type(), tADD);
        }
        if (node->op() == tDECRSET) {
            performArithmeticOperation(node->var()->type(), tSUB);
        }
        storeVarValue(node->var()->name(), node->var()->type());
    }

    void ToBytecodeVisitor::visitStringLiteralNode(StringLiteralNode *node) {
        bytecode->addInsn(BC_SLOAD);
        uint16_t cid = code->makeStringConstant(node->literal());
        bytecode->addUInt16(cid);
        topType = VT_STRING;
    }

    void ToBytecodeVisitor::visitWhileNode(WhileNode *node) {
        Label checkLabel = Label{bytecode};
        Label endLabel = Label{bytecode};

        bytecode->bind(checkLabel);
        node->whileExpr()->visit(this);
        castToType(VT_INT);

        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_IFICMPE, endLabel);

        node->loopBlock()->visit(this);

        bytecode->addBranch(BC_JA, checkLabel);
        bytecode->bind(endLabel);

        topType = VT_VOID;
    }

    void ToBytecodeVisitor::visitIntLiteralNode(IntLiteralNode *node) {
        bytecode->addInsn(BC_ILOAD);
        bytecode->addInt64(node->literal());
        topType = VT_INT;
    }

    void ToBytecodeVisitor::visitBinaryOpNode(BinaryOpNode *node) {
        node->left()->visit(this);
        VarType leftType = topType;
        node->right()->visit(this);

        if (leftType == VT_INT && topType == VT_DOUBLE) {
            bytecode->addInsn(BC_SWAP);
            bytecode->addInsn(BC_I2D);
            bytecode->addInsn(BC_SWAP);
            leftType = VT_DOUBLE;
        } else {
            castToType(leftType);
        }

        switch (node->kind()) {
            case tADD:
            case tSUB:
            case tDIV:
            case tMUL:
            case tMOD:
                performArithmeticOperation(leftType, node->kind());
                break;
            case tAND:
            case tOR:
            case tAAND:
            case tAOR:
            case tAXOR:
                performLogicOperation(leftType, node->kind());
                break;
            case tEQ:
            case tNEQ:
            case tGE:
            case tLE:
            case tGT:
            case tLT:
                performCmpOperation(leftType, node->kind());
                break;
            default:
                throw TranslationException{"No such binary operation."};
        }
    }

    void ToBytecodeVisitor::visitUnaryOpNode(UnaryOpNode *node) {
        if (node->kind() == tNOT) {
            node->operand()->visit(this);
            castToType(VT_INT);

            bytecode->addInsn(BC_ILOAD1);
            bytecode->addInsn(BC_IAXOR);

        } else if (node->kind() == tSUB) {
            node->operand()->visit(this);

            switch (topType) {
                case VT_DOUBLE:
                    bytecode->addInsn(BC_DNEG);
                    break;
                case VT_STRING:
                    castToType(VT_INT);
                case VT_INT:
                    bytecode->addInsn(BC_INEG);
                    break;
                default:
                    throw TranslationException{"Can not negate"};
            }
        } else {
            throw TranslationException{"No such unary operation!"};
        }
    }

    void ToBytecodeVisitor::visitNativeCallNode(NativeCallNode *node) {
        // TODO
    }

    void ToBytecodeVisitor::visitReturnNode(ReturnNode *node) {
        if (node->returnExpr()) {
            node->returnExpr()->visit(this);
            castToType(curFunction->returnType());
        }
        bytecode->addInsn(BC_RETURN);
    }

    void ToBytecodeVisitor::visitFunctionNode(FunctionNode *node) {
        uint16_t oldLocalCount = localCount;
        Bytecode *oldBytecode = bytecode;
        TranslatedFunction *oldFunction = curFunction;
        Scope *oldScope = curScope;

        localCount = 0;
        curFunction = code->functionByName(node->name());
        bytecode = ((BytecodeFunction *) curFunction)->bytecode();
        curScope = node->body()->scope()->parent();
        for (uint32_t i = node->parametersNumber(); i > 0; --i) {
            node->body()->scope()->parent()
                    ->lookupVariable(node->parameterName(i - 1), false)
                    ->setInfo(new VarInfo(curFunction->id(), localCount++));
            topType = node->parameterType(i - 1);
            storeVarValue(node->parameterName(i - 1), node->parameterType(i - 1));
        }


        node->body()->visit(this);
        curFunction->setLocalsNumber(localCount);

        localCount = oldLocalCount;
        curFunction = oldFunction;
        bytecode = oldBytecode;
        curScope = oldScope;
    }

    void ToBytecodeVisitor::visitBlockNode(BlockNode *node) {
        auto oldScope = curScope;
        curScope = node->scope();
        Scope::VarIterator varIterator(node->scope());
        while (varIterator.hasNext()) {
            auto nextVar = varIterator.next();
            nextVar->setInfo(new VarInfo(curFunction->id(), localCount++));
        }
        {
            Scope::FunctionIterator functionIterator(node->scope());
            while (functionIterator.hasNext()) {
                auto function = functionIterator.next();
                BytecodeFunction *tFunction = new BytecodeFunction(function);
                code->addFunction(tFunction);
            }
        }
        {
            Scope::FunctionIterator functionIterator(node->scope());
            while (functionIterator.hasNext()) {
                auto function = functionIterator.next();
                function->node()->visit(this);
            }
        }


        node->visitChildren(this);
        curScope = oldScope;
    }

    void ToBytecodeVisitor::loadVarValue(const std::string &name, VarType varType) {
        uint16_t id;
        uint16_t funcId;
        resolveName(name, funcId, id);

        bool isLocal = curFunction->id() == funcId;

        switch (varType) {
            case VT_INT:
                bytecode->addInsn(isLocal ? BC_LOADIVAR : BC_LOADCTXIVAR);
                break;
            case VT_DOUBLE:
                bytecode->addInsn(isLocal ? BC_LOADDVAR : BC_LOADCTXDVAR);
                break;
            case VT_STRING:
                bytecode->addInsn(isLocal ? BC_LOADSVAR : BC_LOADCTXSVAR);
                break;
            default:
                throw TranslationException{"Not valid type for load"};
        }
        if (!isLocal) {
            bytecode->addUInt16(funcId);
        }

        bytecode->addUInt16(id);
        topType = varType;
    }


    void ToBytecodeVisitor::storeVarValue(const std::string &name, VarType varType) {
        uint16_t id;
        uint16_t funcId;
        resolveName(name, funcId, id);

        bool isLocal = curFunction->id() == funcId;

        if (topType != varType)
            throw TranslationException{
                    "Var type and top stack type is incompatible " + string(typeToName(varType)) + " " +
                    typeToName(topType)};
        switch (topType) {
            case VT_INT:
                bytecode->addInsn(isLocal ? BC_STOREIVAR : BC_STORECTXIVAR);
                break;
            case VT_DOUBLE:
                bytecode->addInsn(isLocal ? BC_STOREDVAR : BC_STORECTXDVAR);
                break;
            case VT_STRING:
                bytecode->addInsn(isLocal ? BC_STORESVAR : BC_STORECTXSVAR);
                break;
            default:
                throw TranslationException{"No such type for store variable: " + string(typeToName(topType))};
        }
        if (!isLocal) {
            bytecode->addUInt16(funcId);
        }

        bytecode->addUInt16(id);
        topType = VT_VOID;
    }

    void ToBytecodeVisitor::print(AstNode *node) {
        node->visit(this);
        switch (topType) {
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
                throw TranslationException{"Not valid type for print"};
        }
        topType = VT_VOID;
    }

    void ToBytecodeVisitor::castToType(VarType type) {
        if (type == topType) {
            return;
        }
        switch (topType) {
            case VT_INT:
                switch (type) {
                    case VT_DOUBLE:
                        bytecode->addInsn(BC_I2D);
                        topType = VT_DOUBLE;
                        break;
                    default:
                        throw TranslationException{
                                "Cast exception from " + string(typeToName(topType)) + " to " + typeToName(type)};
                }
                break;
            case VT_DOUBLE:
                switch (type) {
                    case VT_INT:
                        bytecode->addInsn(BC_D2I);
                        topType = VT_INT;
                        break;
                    default:
                        throw TranslationException{
                                "Cast exception from " + string(typeToName(topType)) + " to " + typeToName(type)};
                }
                break;
            case VT_STRING:
                switch (type) {
                    case VT_INT:
                        bytecode->addInsn(BC_S2I);
                        topType = VT_INT;
                        break;
                    default:
                        throw TranslationException{
                                "Cast exception from " + string(typeToName(topType)) + " to " + typeToName(type)};
                }
                break;
            default:
                throw TranslationException{
                        "Cast exception from " + string(typeToName(topType)) + " to " + typeToName(type)};
        }
    }

    ToBytecodeVisitor::ToBytecodeVisitor(Code *code, Bytecode *bytecode, TranslatedFunction *curFunction)
            : code(code), bytecode(bytecode),
              curFunction(curFunction),
              topType(VT_INVALID) {

    }

    void ToBytecodeVisitor::resolveName(const std::string &name, uint16_t &funcId, uint16_t &varId) {
        auto it = curScope->lookupVariable(name, true);
        if (it == nullptr) {
            throw TranslationException{"No such variable"};
        }
        VarInfo *varInfo = (VarInfo *) it->info();
        funcId = varInfo->funcId;
        varId = varInfo->varId;
    }

    void ToBytecodeVisitor::performArithmeticOperation(VarType type, TokenKind operation) {
        switch (type) {
            case VT_INT:
                switch (operation) {
                    case tADD:
                        bytecode->addInsn(BC_IADD);
                        break;
                    case tSUB:
                        bytecode->addInsn(BC_ISUB);
                        break;
                    case tMUL:
                        bytecode->addInsn(BC_IMUL);
                        break;
                    case tDIV:
                        bytecode->addInsn(BC_IDIV);
                        break;
                    case tMOD:
                        bytecode->addInsn(BC_IMOD);
                        break;
                    default:
                        throw TranslationException{"Invalid type of arithmetic operation!"};
                }
                topType = VT_INT;
                break;
            case VT_DOUBLE:
                switch (operation) {
                    case tADD:
                        bytecode->addInsn(BC_DADD);
                        break;
                    case tSUB:
                        bytecode->addInsn(BC_DSUB);
                        break;
                    case tMUL:
                        bytecode->addInsn(BC_DMUL);
                        break;
                    case tDIV:
                        bytecode->addInsn(BC_DDIV);
                        break;
                    default:
                        throw TranslationException{"Invalid type of arithmetic operation!"};
                }
                topType = VT_DOUBLE;
                break;
            default:
                throw TranslationException{"Invalid type for arithmetic operation!"};
        }
    }

    void ToBytecodeVisitor::performLogicOperation(VarType type, TokenKind operation) {
        if (type == VT_INT) {
            switch (operation) {
                case tAAND:
                case tAND:
                    bytecode->addInsn(BC_IAAND);
                    break;
                case tOR:
                case tAOR:
                    bytecode->addInsn(BC_IAOR);
                    break;
                case tAXOR:
                    bytecode->addInsn(BC_IAXOR);
                    break;
                default:
                    throw TranslationException{"No such logic operation."};
            }
            topType = VT_INT;
        } else {
            throw TranslationException{"Logic operation available for int only."};
        }
    }

    void ToBytecodeVisitor::performCmpOperation(VarType type, TokenKind operation) {
        Instruction instruction;
        switch (operation) {
            case tEQ:
                instruction = BC_IFICMPE;
                break;
            case tNEQ:
                instruction = BC_IFICMPNE;
                break;
            case tGE:
                instruction = BC_IFICMPGE;
                break;
            case tLE:
                instruction = BC_IFICMPLE;
                break;
            case tGT:
                instruction = BC_IFICMPG;
                break;
            case tLT:
                instruction = BC_IFICMPL;
                break;
            default:
                throw TranslationException("Invalid compare operation");
        }
        Label trueLabel = Label{bytecode};
        Label endLabel = Label{bytecode};

        bytecode->addBranch(instruction, trueLabel);
        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_JA, endLabel);
        bytecode->bind(trueLabel);
        bytecode->addInsn(BC_ILOAD1);
        bytecode->bind(endLabel);

        topType = VT_INT;
    }


    TranslationException::TranslationException(const string &__arg) : runtime_error(__arg) {}
}
