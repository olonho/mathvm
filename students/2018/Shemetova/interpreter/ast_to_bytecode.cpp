#include "ast_to_bytecode.h"
#include "translator_helper.h"

namespace mathvm {

    VarType AstToBytecodeVisitor::get_type(AstNode* node) {
        return *((VarType*) node->info()); //taken from StorageManager
    }
    

    void AstToBytecodeVisitor::typevar_converter(VarType first, VarType second) {
        if (first == VT_INT && second == VT_DOUBLE) {
            bytecode->addInsn(BC_I2D);
            return;
        }
        if (first == VT_DOUBLE && second == VT_INT) {
            bytecode->addInsn(BC_D2I);
            return;
        }
        if (first == VT_STRING && second == VT_INT) {
            bytecode->addInsn(BC_S2I);
            return;
        }
    }

    void AstToBytecodeVisitor::boolean_converter() {
        Label first(bytecode);
        Label second(bytecode);
        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_IFICMPE, first);
        bytecode->addInsn(BC_ILOAD1);
        bytecode->addBranch(BC_JA, second);
        bytecode->bind(first);
        bytecode->addInsn(BC_ILOAD0);
        bytecode->bind(second);
    }

    void AstToBytecodeVisitor::makeNotExpr() {
        Label elseLabel(bytecode);
        Label endLabel(bytecode);
        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_IFICMPNE, elseLabel);
        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_JA, endLabel);
        bytecode->bind(elseLabel);
        bytecode->addInsn(BC_ILOAD1);
        bytecode->bind(endLabel);
    }

    void AstToBytecodeVisitor::makeArithmetic(BinaryOpNode* node) {
        VarType result = get_type(node);
        (node->left())->visit(this);
        typevar_converter(get_type(node->left()), result);
        (node->right())->visit(this);
        typevar_converter(get_type(node->right()), result);
        bytecode->addInsn(BC_SWAP);
        TokenKind token = node->kind();
        if (result == VT_INT) {
            switch (token) {
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
                default: break;
            }
        } else if (result == VT_DOUBLE) {
            switch (token) {
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
                default: break;
                    /*
                case tMOD: //maybe do some casting?
                    bytecode->add(BC_IMOD);
                    break;
                     */
            }
        }
    }

    void AstToBytecodeVisitor::makeBitwise(BinaryOpNode* node) {
        //TODO : some casting
        /*
        (node->left())->visit(this);
        typevar_converter(get_type(node->left), VT_INT);
        (node->right())->visit(this);
        typevar_converter(get_type(node->left), VT_INT);
         */
        TokenKind token = node->kind();
        (node->left())->visit(this);
        (node->right())->visit(this);
        switch (token) {
            case tAOR:
                bytecode->addInsn(BC_IAOR);
                break;
            case tAAND:
                bytecode->addInsn(BC_IAAND);
                break;
            case tAXOR:
                bytecode->addInsn(BC_IAXOR);
                break;

            default: break;
        }
    }

    void AstToBytecodeVisitor::makeBoolean(BinaryOpNode* node) {
        TokenKind token = node->kind();
        (node->right())->visit(this);
        boolean_converter();
        (node->left())->visit(this);
        boolean_converter();
        //bytecode->addInsn(BC_SWAP);
        switch (token) {
            case tAND:
                bytecode->addInsn(BC_IAAND);
                break;
            case tOR:
                bytecode->addInsn(BC_IAOR);
                break;
            default: break;
        }
    }

    void AstToBytecodeVisitor::makeComparision(BinaryOpNode* node) {
        VarType result;
        VarType l = get_type(node->left());
        VarType r = get_type(node->right());
        if (l == VT_DOUBLE || r == VT_DOUBLE) {
            result = VT_DOUBLE;
        } else {
            result = VT_INT;
        }


        (node->right())->visit(this);
        typevar_converter(r, result);

        (node->left())->visit(this);
        typevar_converter(l, result);

        if (result == VT_INT) {
            bytecode->addInsn(BC_ICMP);
        } else if (result == VT_DOUBLE) {
            bytecode->addInsn(BC_DCMP);
        }
        bytecode->addInsn(BC_ILOAD0);

        Label elseLabel(bytecode);
        Label endLabel(bytecode);

        switch (node->kind()) {
            case tEQ: bytecode->addBranch(BC_IFICMPE, elseLabel);
                break;
            case tNEQ: bytecode->addBranch(BC_IFICMPNE, elseLabel);
                break;
            case tGT:bytecode->addBranch(BC_IFICMPG, elseLabel);
                break;
            case tGE:bytecode->addBranch(BC_IFICMPGE, elseLabel);
                break;
            case tLT:bytecode->addBranch(BC_IFICMPL, elseLabel);
                break;
            case tLE: bytecode->addBranch(BC_IFICMPLE, elseLabel);
                break;
            default: break;
                //default: return
        }
        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_JA, endLabel);

        bytecode->bind(elseLabel);
        bytecode->addInsn(BC_ILOAD1);
        bytecode->bind(endLabel);
        
    }

    void AstToBytecodeVisitor::makeStore(const AstVar* varS) {
         if (ScopeHelper::checkVarVisibility(varS, currentScope)) {
            const AstVar* var = currentScope->lookupVariable(varS->name());
            //local rewriting
            if (!var) {
                var = varS;
            }
            uint16_t scopeId = helper.getScopeId(var->owner());
            uint16_t varId = helper.getVarId(var);
            if (var->owner() == currentScope) {
                switch (var->type()) {
                    case VT_DOUBLE:
                        bytecode->addInsn(BC_STOREDVAR);
                        break;
                    case VT_INT:
                        bytecode->addInsn(BC_STOREIVAR);
                        break;
                    case VT_STRING:
                        bytecode->addInsn(BC_STORESVAR);
                        break;
                    default:break;
                }
            } else {
                switch (var->type()) {
                    case VT_DOUBLE:
                        bytecode->addInsn(BC_STORECTXDVAR);
                        break;
                    case VT_INT:
                        bytecode->addInsn(BC_STORECTXIVAR);
                        break;
                    case VT_STRING:
                        bytecode->addInsn(BC_STORECTXSVAR);
                        break;
                    default:break;
                }
                bytecode->addInt16(scopeId);
            }
            bytecode->addInt16(varId);
        } else {
            cout << "Translator error: No var named " + varS->name() << endl;
            throw std::runtime_error("Translator error: No var named " + varS->name());
        }
    }

    void AstToBytecodeVisitor::makeLoad(const AstVar* varL) {
        if (ScopeHelper::checkVarVisibility(varL, currentScope)) {
            const AstVar* var = currentScope->lookupVariable(varL->name());
            //local rewriting
            if (!var) {
                var = varL;
            }
            uint16_t scopeId = helper.getScopeId(var->owner());
            uint16_t varId = helper.getVarId(var);
            if (var->owner() == currentScope) {
                switch (var->type()) {
                    case VT_DOUBLE:
                        bytecode->addInsn(BC_LOADDVAR);
                        break;
                    case VT_INT:
                        bytecode->addInsn(BC_LOADIVAR);
                        break;
                    case VT_STRING:
                        bytecode->addInsn(BC_LOADSVAR);
                        break;
                    default:break;
                }
            } else {
                switch (var->type()) {
                    case VT_DOUBLE:
                        bytecode->addInsn(BC_LOADCTXDVAR);
                        break;
                    case VT_INT:
                        bytecode->addInsn(BC_LOADCTXIVAR);
                        break;
                    case VT_STRING:
                        bytecode->addInsn(BC_LOADCTXSVAR);
                        break;
                    default:break;
                }
                bytecode->addInt16(scopeId);
            }
            bytecode->addInt16(varId);
        } else {
            cout << "Translator error: No var named " << varL->name() << endl;
            throw std::runtime_error("Translator error: No var named " + varL->name());
        }
    }

    void AstToBytecodeVisitor::visitIntLiteralNode(IntLiteralNode * node) {

        bytecode->addInsn(BC_ILOAD);
        bytecode->addInt64(node->literal());
    }

    void AstToBytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode * node) {

        bytecode->addInsn(BC_DLOAD);
        bytecode->addDouble(node->literal());
    }

    void AstToBytecodeVisitor::visitStringLiteralNode(StringLiteralNode * node) {
        const string& value = node->literal();
        if (value.empty()) {
            bytecode->add(BC_SLOAD0);
        } else {

            bytecode->add(BC_SLOAD);
            uint16_t i = code->makeStringConstant(value);
            bytecode->addUInt16(i);
        }
    }

    /*
    DO(LOADDVAR, "Load double from variable, whose 2-byte is id inlined to insn stream, push on TOS.", 3) \
    DO(LOADIVAR, "Load int from variable, whose 2-byte id is inlined to insn stream, push on TOS.", 3) \
    DO(LOADSVAR, "Load string from variable, whose 2-byte id is inlined to insn stream, push on TOS.", 3) \
    DO(LOADCTXDVAR, "Load double from variable, whose 2-byte context and 2-byte id inlined to insn stream, push on TOS.", 5) \
    DO(LOADCTXIVAR, "Load int from variable, whose 2-byte context and 2-byte id is inlined to insn stream, push on TOS.", 5) \
    DO(LOADCTXSVAR, "Load string from variable, whose 2-byte context and 2-byte id is inlined to insn stream, push on TOS.", 5) \
         
     */
    void AstToBytecodeVisitor::visitLoadNode(LoadNode* node) {
        const AstVar* var = node->var();
        makeLoad(var);
    }

    void AstToBytecodeVisitor::visitBinaryOpNode(BinaryOpNode* node) {
        TokenKind token = node->kind();
        switch (token) {
            case tADD:
            case tSUB:
            case tMUL:
            case tDIV:
            case tMOD:
                makeArithmetic(node);
                break;
            case tAND:
            case tOR:
                makeBoolean(node);
                break;
            case tEQ:
            case tNEQ:
            case tGT:
            case tGE:
            case tLT:
            case tLE:
                makeComparision(node);
                node->setInfo(new VarType(VT_INT)); //fix boolean
                break;
            case tAOR:
            case tAAND:
            case tAXOR:
                makeBitwise(node);

                break;
            default: break;
        }
    }

    void AstToBytecodeVisitor::visitUnaryOpNode(UnaryOpNode * node) {
        VarType type = get_type(node);
        TokenKind token = node->kind();
        node->operand()->visit(this);
        switch (token) {
            case tNOT:
                //check int
                bytecode->addInsn(BC_ILOAD1);
                bytecode->addInsn(BC_IAXOR);
                //makeNotExpr();
                break;
            case tSUB:
                if (type == VT_INT) {
                    bytecode->addInsn(BC_INEG);
                } else {

                    bytecode->addInsn(BC_DNEG);
                }
                break;
            default: break;
        }
    }

    void AstToBytecodeVisitor::visitPrintNode(PrintNode * node) {
        for (uint32_t i = 0; i < node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            VarType arg_type = get_type(node->operandAt(i));
            switch (arg_type) {
                case VT_DOUBLE:
                    bytecode->addInsn(BC_DPRINT);
                    break;
                case VT_INT:
                    bytecode->addInsn(BC_IPRINT);
                    break;
                case VT_STRING:
                    bytecode->addInsn(BC_SPRINT);

                    break;
                default: break;
            }
        }
    }

    //DO(CALL, "Call function, next two bytes - unsigned function id.", 3)

    void AstToBytecodeVisitor::visitCallNode(CallNode * node) {
        AstFunction* func = currentScope->lookupFunction(node->name());
        if (func) {
            uint32_t n = node->parametersNumber();
            //backward!!!
            for (uint32_t i = n; i > 0; --i) {
                AstNode* current = node->parameterAt(i - 1);
                current->visit(this);
                typevar_converter(get_type(current), func->parameterType(i - 1));
            }
            bytecode->addInsn(BC_CALL);
            bytecode->addUInt16(code->functionByName(node->name())->id());
        } else {
            cout << "No such function!" << endl;
            throw std::runtime_error("No such function!");
        }
    }

    void AstToBytecodeVisitor::visitMain(const AstFunction* main) {
        assert(main->name() == "<top>");
        BytecodeFunction* tf = new BytecodeFunction(const_cast<AstFunction*> (main));
        helper.registerScope(main->node()->body()->scope(), 0);
        code->addFunction(tf);
        bytecode = tf->bytecode();
        ScopeHelper::updateFunctionInfo(code, main->node());
        main->node()->visit(this);
    }

    void AstToBytecodeVisitor::visitFunctionNode(FunctionNode* node) {      
        if (node->name() == "<top>") {
            node->body()->visit(this);
            bytecode->addInsn(BC_STOP);
        } else {
            BytecodeFunction* func = dynamic_cast<BytecodeFunction*> (code->functionByName(node->name()));
            Bytecode* prev = bytecode;
            ScopeHelper::updateFunctionInfo(code, node);
            bytecode = func->bytecode();
            Scope* previous = currentScope;
            currentScope = node->body()->scope();
            for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
                currentScope->declareVariable(node->parameterName(i), node->parameterType(i));
                AstVar* var = currentScope->lookupVariable(node->parameterName(i));
                if (var) {
                    var->setInfo(new VarType(var->type()));
                    makeStore(var);
                }
                else {
                    cout << "omg" << endl;
                }
            }
            currentScope = previous;
            node->body()->visit(this);
            bytecode = prev;
        }
    }

    void AstToBytecodeVisitor::visitBlockNode(BlockNode* node) {
        Scope* previous = currentScope;
        currentScope = node->scope();
        //make setLocalVariables? (if it needs unique id)
        if (!helper.checkScope(currentScope)) {
            helper.registerScope(currentScope);
        }
        /*
        Scope::VarIterator itv = Scope::VarIterator(currentScope);
        while (itv.hasNext()) {
            AstVar* var = itv.next();
            cout << "var_is" << var->name() << endl;
            //var->setInfo(new VarType(var->type()));
            //makeStore(var);
        }*/
        
        helper.setFunctions(code, currentScope);
        
        Scope::FunctionIterator it = Scope::FunctionIterator(currentScope);
        while (it.hasNext()) {
            AstFunction* func = it.next();
            func->node()->visit(this);
        }
        
        
        node->visitChildren(this);
        currentScope = previous;
    }

    void AstToBytecodeVisitor::visitStoreNode(StoreNode * node) {
        node->value()->visit(this);
        const AstVar* var = node->var();
        VarType type = var->type();
        typevar_converter(get_type(node->value()), type);
        switch (node->op()) {
            case tINCRSET:
                makeLoad(var);
                switch (type) {
                    case VT_INT:
                        bytecode->addInsn(BC_IADD);
                        break;
                    case VT_DOUBLE:
                        bytecode->addInsn(BC_DADD);
                        break;
                    default:break;
                }
                makeStore(var);
                break;

            case tDECRSET:
                makeLoad(var);
                switch (type) {
                    case VT_INT:
                        bytecode->addInsn(BC_ISUB);
                        break;
                    case VT_DOUBLE:
                        bytecode->addInsn(BC_DSUB);
                        break;
                    default:break;
                }
                makeStore(var);

                break;
            case tASSIGN:
                makeStore(var);
            default: break;
        }
    }

    void AstToBytecodeVisitor::visitForNode(ForNode * node) {
        BinaryOpNode* in = node->inExpr()->asBinaryOpNode();
        const AstVar* var = node->var();
        VarType result = var->type();
        in->left()->visit(this);
        typevar_converter(get_type(in->left()), result);

        makeStore(var);
        Label startLabel(bytecode);
        Label endLabel(bytecode);
        bytecode->bind(startLabel);
        makeLoad(var);
        in->right()->visit(this);
        bytecode->addBranch(BC_IFICMPG, endLabel);
        node->body()->visit(this);
        makeLoad(var);
        bytecode->addInsn(BC_ILOAD1);
        bytecode->addInsn(BC_IADD);
        makeStore(var);
        bytecode->addBranch(BC_JA, startLabel);
        bytecode->bind(endLabel);
    }

    void AstToBytecodeVisitor::visitIfNode(IfNode * node) {
        node->ifExpr()->visit(this);

        Label elseBlock(bytecode);
        Label end(bytecode);

        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_IFICMPE, elseBlock);

        node->thenBlock()->visit(this);

        bytecode->addBranch(BC_JA, end);
        bytecode->bind(elseBlock);

        if (node->elseBlock() != nullptr) {
            node->elseBlock()->visit(this);
        }
        bytecode->bind(end);
    }

    void AstToBytecodeVisitor::visitWhileNode(WhileNode * node) {

        Label loopLabel(bytecode);
        Label endLabel(bytecode);
        bytecode->bind(loopLabel);
        (node->whileExpr())->visit(this);
        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_IFICMPE, endLabel);
        (node->loopBlock())->visit(this);
        bytecode->addBranch(BC_JA, loopLabel);
        bytecode->bind(endLabel);
    }

    void AstToBytecodeVisitor::visitReturnNode(ReturnNode * node) {
        AstNode* returned = node->returnExpr();
        if (returned) {
            returned->visit(this);
            typevar_converter(get_type(returned), get_type(node));
        }
        bytecode->addInsn(BC_RETURN);

    }

}

