//
// Created by aleks on 25.11.18.
//

#include "bytecode.h"
#include "typeinference.h"

namespace mathvm {

#define get_info(node) (reinterpret_cast<Info*>(node->info()))

    BytecodeVisitor::BytecodeVisitor(Code *code) : _code(code) {}

    void BytecodeVisitor::setLocalsInfo(BytecodeFunction *function) {
        setLocalsInfo();
        function->setLocalsNumber(var_number + 1);
    };

    void BytecodeVisitor::setLocalsInfo() {
        Scope::VarIterator it(cur_scope);
        while (it.hasNext()) {
            AstVar *pVar = it.next();
            Info *info = get_info(pVar);
            info->setId(var_number);
            info->setContextId(scope_id);
            ++var_number;
        }
        _cur_function->setLocalsNumber(var_number + _cur_function->localsNumber());
    };

    void BytecodeVisitor::storeVariable(const AstVar *var) {
        Info *info = get_info(var);

        uint16_t context_id = info->getContextId();
        uint16_t variable_id = info->getId();
        switch (var->type()) {
            case VT_INT:
                storeIntVariable(variable_id, context_id);
                break;
            case VT_DOUBLE:
                storeDoubleVariable(variable_id, context_id);
                break;
            case VT_STRING:
                storeStringVariable(variable_id, context_id);
                break;
            default:
                assert(0);

        }
    };

    void BytecodeVisitor::storeIntVariable(uint16_t varId, uint16_t contextId) {
        if (contextId != scope_id) {
            bytecode->add(BC_STORECTXIVAR);
            bytecode->addUInt16(contextId);
            bytecode->addUInt16(varId);
            return;
        }

        switch (varId) {
            case 0:
                bytecode->add(BC_STOREIVAR0);
                break;
            case 1:
                bytecode->add(BC_STOREIVAR1);
                break;
            case 2:
                bytecode->add(BC_STOREIVAR2);
                break;
            case 3:
                bytecode->add(BC_STOREIVAR3);
                break;
            default:
                bytecode->add(BC_STOREIVAR);
                bytecode->addUInt16(varId);
                break;
        }
    }

    void BytecodeVisitor::storeDoubleVariable(uint16_t varId, uint16_t contextId) {
        if (contextId != scope_id) {
            bytecode->add(BC_STORECTXDVAR);
            bytecode->addUInt16(contextId);
            bytecode->addUInt16(varId);
            return;
        }

        switch (varId) {
            case 0:
                bytecode->add(BC_STOREDVAR0);
                break;
            case 1:
                bytecode->add(BC_STOREDVAR1);
                break;
            case 2:
                bytecode->add(BC_STOREDVAR2);
                break;
            case 3:
                bytecode->add(BC_STOREDVAR3);
                break;
            default:
                bytecode->add(BC_STOREDVAR);
                bytecode->addUInt16(varId);
                break;
        }
    }

    void BytecodeVisitor::storeStringVariable(uint16_t varId, uint16_t contextId) {
        if (contextId != scope_id) {
            bytecode->add(BC_STORECTXSVAR);
            bytecode->addUInt16(contextId);
            bytecode->addUInt16(varId);
            return;
        }

        switch (varId) {
            case 0:
                bytecode->add(BC_STORESVAR0);
                break;
            case 1:
                bytecode->add(BC_STORESVAR1);
                break;
            case 2:
                bytecode->add(BC_STORESVAR2);
                break;
            case 3:
                bytecode->add(BC_STORESVAR3);
                break;
            default:
                bytecode->add(BC_STORESVAR);
                bytecode->addUInt16(varId);
                break;
        }
    }


    void BytecodeVisitor::loadVariable(const mathvm::AstVar *var) {
        Info *info = get_info(var);

        uint16_t context_id = info->getContextId();
        uint16_t variable_id = info->getId();
        switch (info->getType()) {
            case VT_INT:
                loadIntVariable(variable_id, context_id);
                break;
            case VT_DOUBLE:
                loadDoubleVariable(variable_id, context_id);
                break;
            case VT_STRING:
                loadStringVariable(variable_id, context_id);
                break;
            default:
                assert(0);
        }
    }

    void BytecodeVisitor::loadIntVariable(uint16_t varId, uint16_t contextId) {
        if (contextId != scope_id) {
            bytecode->add(BC_LOADCTXIVAR);
            bytecode->addUInt16(contextId);
            bytecode->addUInt16(varId);
            return;
        }

        switch (varId) {
            case 0:
                bytecode->add(BC_LOADIVAR0);
                break;
            case 1:
                bytecode->add(BC_LOADIVAR1);
                break;
            case 2:
                bytecode->add(BC_LOADIVAR2);
                break;
            case 3:
                bytecode->add(BC_LOADIVAR3);
                break;
            default:
                bytecode->add(BC_LOADIVAR);
                bytecode->addUInt16(varId);
                break;
        }
    }

    void BytecodeVisitor::loadDoubleVariable(uint16_t varId, uint16_t contextId) {
        if (contextId != scope_id) {
            bytecode->add(BC_LOADCTXDVAR);
            bytecode->addUInt16(contextId);
            bytecode->addUInt16(varId);
            return;
        }

        switch (varId) {
            case 0:
                bytecode->add(BC_LOADDVAR0);
                break;
            case 1:
                bytecode->add(BC_LOADDVAR1);
                break;
            case 2:
                bytecode->add(BC_LOADDVAR2);
                break;
            case 3:
                bytecode->add(BC_LOADDVAR3);
                break;
            default:
                bytecode->add(BC_LOADDVAR);
                bytecode->addUInt16(varId);
                break;
        }

    };

    void BytecodeVisitor::loadStringVariable(uint16_t varId, uint16_t contextId) {
        if (contextId != scope_id) {
            bytecode->add(BC_LOADCTXSVAR);
            bytecode->addUInt16(contextId);
            bytecode->addUInt16(varId);
            return;
        }

        switch (varId) {
            case 0:
                bytecode->add(BC_LOADSVAR0);
                break;
            case 1:
                bytecode->add(BC_LOADSVAR1);
                break;
            case 2:
                bytecode->add(BC_LOADSVAR2);
                break;
            case 3:
                bytecode->add(BC_LOADSVAR3);
                break;
            default:
                bytecode->add(BC_LOADSVAR);
                bytecode->addUInt16(varId);
                break;
        }
    };

    void BytecodeVisitor::visitBlockNode(BlockNode *node) {
        uint32_t nodeNum = node->nodes();

        Scope *oldScope = cur_scope;
        cur_scope = node->scope();

        setLocalsInfo();

        Scope::FunctionIterator functionIt(node->scope());
        while (functionIt.hasNext()) {
            AstFunction *function = functionIt.next();
            registerFunction(function);
        }


        Scope::FunctionIterator functionIterator(node->scope());
        while (functionIterator.hasNext()) {
            AstFunction *function = functionIterator.next();
            visitAstFunction(function);
        }

        for (uint32_t i = 0; i < nodeNum; ++i) {
            AstNode *pNode = node->nodeAt(i);
            pNode->visit(this);

            // TODO mb delete this
            if (pNode->isReturnNode() && pNode->asReturnNode()->returnExpr() == nullptr)
                continue;

            if (get_info(pNode)->getType() != VT_VOID) {
                bytecode->addInsn(BC_POP);
            }
        }

        cur_scope = oldScope;
    }

    void BytecodeVisitor::registerFunction(AstFunction *function) {
        auto funcToStore = new BytecodeFunction(function);
        uint16_t id = _code->addFunction(funcToStore);
        funcToStore->setScopeId(id);
    }

    void BytecodeVisitor::visitAstFunction(mathvm::AstFunction *astFunction) {
        BytecodeFunction *oldFunction = _cur_function;
        Bytecode *oldBytecode = bytecode;
        uint16_t oldScopeId = scope_id;
        Scope *oldScope = astFunction->node()->body()->scope();

        _cur_function = (BytecodeFunction *) _code->functionByName(astFunction->name());
        scope_id = _cur_function->scopeId();
        bytecode = _cur_function->bytecode();
        cur_scope = astFunction->scope();
        var_number = 0;


        setLocalsInfo(_cur_function);
        for (uint i = 0; i < astFunction->parametersNumber(); ++i) {
            AstVar *var = cur_scope->lookupVariable(astFunction->parameterName(i), false);
            storeVariable(var);
        }

        astFunction->node()->visit(this);

        if (astFunction->name() == "<top>") {
            bytecode->addInsn(BC_STOP);
        }

        cur_scope = oldScope;
        scope_id = oldScopeId;
        bytecode = oldBytecode;
        _cur_function = oldFunction;
    }

    void BytecodeVisitor::visitFunctionNode(mathvm::FunctionNode *node) {
        node->body()->visit(this);
    }

    void BytecodeVisitor::visitIntLiteralNode(mathvm::IntLiteralNode *node) {
        int64_t val = node->literal();
        switch (val) {
            case 0:
                bytecode->add(BC_ILOAD0);
                break;
            case 1:
                bytecode->add(BC_ILOAD1);
                break;
            case -1:
                bytecode->add(BC_ILOADM1);
                break;
            default:
                bytecode->add(BC_ILOAD);
                bytecode->addInt64(val);
                break;
        }
    }


    void BytecodeVisitor::visitPrintNode(mathvm::PrintNode *node) {
        uint32_t opNum = node->operands();
        for (uint32_t i = 0; i < opNum; ++i) {
            AstNode *pNode = node->operandAt(i);
            pNode->visit(this);
            VarType type = get_info(pNode)->getType();
            switch (type) {
                case VT_INVALID:
                case VT_VOID:
                    break;
                case VT_INT:
                    bytecode->add(BC_IPRINT);
                    break;
                case VT_DOUBLE:
                    bytecode->add(BC_DPRINT);
                    break;
                case VT_STRING:
                    bytecode->add(BC_SPRINT);
                    break;
            }
        }
    }

    void BytecodeVisitor::visitCallNode(CallNode *node) {
        uint32_t total = node->parametersNumber() - 1;
        AstFunction *pFunction = cur_scope->lookupFunction(node->name());
        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            uint32_t curId = total - i;
            AstNode *pNode = node->parameterAt(curId);
            pNode->visit(this);
            cast(get_info(pNode)->getType(), pFunction->parameterType(curId));
        }

        bytecode->addInsn(BC_CALL);
        bytecode->addUInt16(_code->functionByName(pFunction->name())->scopeId());
    }

    void BytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
        double val = node->literal();
        if (val == 0) {
            bytecode->add(BC_DLOAD0);
        } else if (val == 1) {
            bytecode->add(BC_DLOAD1);
        } else if (val == -1) {
            bytecode->add(BC_DLOADM1);
        } else {
            bytecode->add(BC_DLOAD);
            bytecode->addDouble(val);
        }
    }

    void BytecodeVisitor::visitStringLiteralNode(StringLiteralNode *node) {
        const string &literal = node->literal();
        if (literal.empty()) {
            bytecode->add(BC_SLOAD0);
        } else {
            bytecode->add(BC_SLOAD);
            uint16_t i = _code->makeStringConstant(literal);
            bytecode->addUInt16(i);
        }
    }

    void BytecodeVisitor::cast(VarType left, VarType right) {
        if (left == right) {
            return;
        }

        if (left == VT_DOUBLE && right == VT_INT) {
            bytecode->add(BC_D2I);
            return;
        }
        if (left == VT_INT && right == VT_DOUBLE) {
            bytecode->add(BC_I2D);
            return;
        }
        if (left == VT_STRING && right == VT_INT) {
            bytecode->add(BC_S2I);
            return;
        }
    }

    void BytecodeVisitor::visitBinaryOpNode(BinaryOpNode *node) {
        TokenKind kind = node->kind();
        switch (kind) {
            case tADD:
            case tSUB:
            case tDIV:
            case tMUL:
            case tMOD:
            case tAAND:
            case tAOR:
            case tAXOR:
                generateArithmetic(node);
                break;

            case tAND:
            case tOR:
                generateBoolean(node);
                break;

            case tEQ:
            case tNEQ:
            case tGT:
            case tGE:
            case tLT:
            case tLE:
                generateComparison(node);
                break;
            default:
                assert(0);
        }
    }

    void BytecodeVisitor::generateComparison(BinaryOpNode *node) {
        VarType leftType = get_info(node->left())->getType();
        VarType rightType = get_info(node->right())->getType();
        VarType type;

        if (leftType == VT_DOUBLE || rightType == VT_DOUBLE) {
            type = VT_DOUBLE;
        } else {
            type = VT_INT;
        }

        node->left()->visit(this);
        cast(leftType, type);

        node->right()->visit(this);
        cast(rightType, type);

        bytecode->addInsn(BC_SWAP);

        if (type == VT_DOUBLE) {
            bytecode->addInsn(BC_DCMP);
        } else if (type == VT_INT) {
            bytecode->addInsn(BC_ICMP);
        }

        generateComparisonSwitching(node->kind());
    }

    void BytecodeVisitor::generateComparisonSwitching(TokenKind kind) {
        bytecode->addInsn(BC_ILOAD0);

        Label elseLabel(bytecode);
        Label endLabel(bytecode);
        switch (kind) {
            case tEQ:
                bytecode->addBranch(BC_IFICMPE, elseLabel);
                break;
            case tNEQ:
                bytecode->addBranch(BC_IFICMPNE, elseLabel);
                break;
            case tGT:
                bytecode->addBranch(BC_IFICMPG, elseLabel);
                break;
            case tGE:
                bytecode->addBranch(BC_IFICMPGE, elseLabel);
                break;
            case tLT:
                bytecode->addBranch(BC_IFICMPL, elseLabel);
                break;
            case tLE:
                bytecode->addBranch(BC_IFICMPLE, elseLabel);
                break;
            default:
                assert(0);
        }

        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_JA, endLabel);

        bytecode->bind(elseLabel);
        bytecode->addInsn(BC_ILOAD1);
        bytecode->bind(endLabel);

    }



    void BytecodeVisitor::generateArithmetic(BinaryOpNode *node) {
        VarType type = get_info(node)->getType();

        node->left()->visit(this);
        cast(get_info(node->left())->getType(), type);

        node->right()->visit(this);
        cast(get_info(node->right())->getType(), type);

        bytecode->addInsn(BC_SWAP);

        TokenKind tokenKind = node->kind();
        if (type == VT_DOUBLE) {
            generateDoubleArithmetic(tokenKind);
        } else if (type == VT_INT) {
            generateIntArithmetic(tokenKind);
        }
    }


    void BytecodeVisitor::generateIntArithmetic(mathvm::TokenKind tokenKind) {
        switch (tokenKind) {
            case tADD:
                bytecode->add(BC_IADD);
                break;
            case tSUB:
                bytecode->add(BC_ISUB);
                break;
            case tDIV:
                bytecode->add(BC_IDIV);
                break;
            case tMUL:
                bytecode->add(BC_IMUL);
                break;
            case tMOD:
                bytecode->add(BC_IMOD);
                break;
            case tAAND:

                bytecode->add(BC_IAAND);
                break;
            case tAOR:
                bytecode->add(BC_IAOR);
                break;
            case tAXOR:
                bytecode->add(BC_IAXOR);
                break;
            default:
                return;
        }
    };

    void BytecodeVisitor::castToBoolean() {
        Label first(bytecode);
        Label second(bytecode);

        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_IFICMPE, first);

        bytecode->addInsn(BC_ILOAD1);
        bytecode->addBranch(BC_JA, second);
        bytecode->bind(first);

        bytecode->addInsn(BC_ILOAD0);
        bytecode->bind(second);
    };

    void BytecodeVisitor::generateBoolean(BinaryOpNode *node) {
        node->left()->visit(this);
        castToBoolean();

        node->right()->visit(this);
        castToBoolean();

        bytecode->addInsn(BC_SWAP);

        // TODO: check types?
        switch (node->kind()) {
            case tAND:
                bytecode->addInsn(BC_IAAND);
                break;

            case tOR:
                bytecode->addInsn(BC_IAOR);
                break;

            default:
                assert(0);
        }
    }

    void BytecodeVisitor::generateDoubleArithmetic(mathvm::TokenKind tokenKind) {
        switch (tokenKind) {
            case tADD:
                bytecode->add(BC_DADD);
                break;
            case tSUB:
                bytecode->add(BC_DSUB);
                break;
            case tDIV:
                bytecode->add(BC_DDIV);
                break;
            case tMUL:
                bytecode->add(BC_DMUL);
                break;
            default:
                return;
        }
    }

    void BytecodeVisitor::visitStoreNode(StoreNode *node) {

        VarType resType = node->var()->type();
        node->value()->visit(this);
        cast(get_info(node->value())->getType(), resType);
        switch (node->op()) {
            case tASSIGN:
                storeVariable(node->var());
                break;

            case tINCRSET:
            case tDECRSET: {
                loadVariable(node->var());
                bytecode->add(node->op() == tINCRSET ? BC_IADD : BC_ISUB);
                storeVariable(node->var());
                break;
            }
            default:
                assert(0);
        }
    }


    void BytecodeVisitor::visitLoadNode(LoadNode *node) {
        loadVariable(node->var());
    }

    void BytecodeVisitor::visitUnaryOpNode(UnaryOpNode *node) {
        VarType type = get_info(node->operand())->getType();
        node->operand()->visit(this);
        TokenKind kind = node->kind();

        switch (kind) {
            case tSUB:
                bytecode->addInsn(type == VT_INT ? BC_INEG : BC_DNEG);
                break;
            case tNOT:
                generateComparisonSwitching(tEQ);
                break;
            default:
                assert(0);

        }
    }

    void BytecodeVisitor::visitForNode(ForNode *node) {
        BinaryOpNode *inExpr = node->inExpr()->asBinaryOpNode();
        const AstVar *forVar = node->var();

        VarType resType = forVar->type();
        inExpr->left()->visit(this);
        cast(get_info(inExpr->left())->getType(), resType);

        storeVariable(forVar);

        Label startLabel(bytecode);
        Label endLabel(bytecode);

        bytecode->bind(startLabel);
        loadVariable(forVar);
        inExpr->right()->visit(this);

        bytecode->addBranch(BC_IFICMPG, endLabel);
        node->body()->visit(this);

        loadVariable(forVar);
        bytecode->addInsn(BC_ILOAD1);
        bytecode->addInsn(BC_IADD);
        storeVariable(forVar);
        bytecode->addBranch(BC_JA, startLabel);
        bytecode->bind(endLabel);
    }

    void BytecodeVisitor::visitIfNode(IfNode *node) {
        node->ifExpr()->visit(this);

        Label first(bytecode);
        Label second(bytecode);

        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_IFICMPE, first);

        node->thenBlock()->visit(this);

        bytecode->addBranch(BC_JA, second);
        bytecode->bind(first);

        if (node->elseBlock() != nullptr) {
            node->elseBlock()->visit(this);
        }
        bytecode->bind(second);
    }


    void BytecodeVisitor::visitWhileNode(WhileNode *node) {
        Label loop = Label(bytecode);
        Label ex = Label(bytecode);
        bytecode->bind(loop);

        node->whileExpr()->visit(this);
        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_IFICMPE, ex);

        node->loopBlock()->visit(this);
        bytecode->addBranch(BC_JA, loop);
        bytecode->bind(ex);
    }

    void BytecodeVisitor::visitNativeCallNode(NativeCallNode *node) {
        // TODO
        AstBaseVisitor::visitNativeCallNode(node);
    }

    void BytecodeVisitor::visitReturnNode(ReturnNode *node) {
        AstNode *pNode = node->returnExpr();
        if (pNode != nullptr) {
            pNode->visit(this);
            cast(get_info(pNode)->getType(), get_info(node)->getType());
        }

        bytecode->addInsn(BC_RETURN);
    };

#undef get_info
}