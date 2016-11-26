#include <dlfcn.h>

#include "ast_to_bytecode.h"

namespace mathvm
{
    void ast_to_bytecode_visitor::translate(Code *code, AstFunction *f)
    {
        _code = code;
        BytecodeFunction* byte_func = new BytecodeFunction(f);
        _code->addFunction(byte_func);

        translate_func(f);

        byte_func->bytecode()->addInsn(BC_STOP);
    }

    void ast_to_bytecode_visitor::translate_func(AstFunction *f)
    {
        BytecodeFunction* byte_func = (BytecodeFunction *) _code->functionByName(f->name());
        uint16_t id = _context.size() > 0 ? _context.front().id() + 1 : 0;

        context new_context(byte_func, f->scope(), id);
        _context.push_front(new_context);

        for (uint i = 0; i < f->parametersNumber(); ++i) 
        {
            store(f->scope()->lookupVariable(f->parameterName(i), false));
        }

        f->node()->visit(this);

        byte_func->setScopeId(_context.front().id());
        byte_func->setLocalsNumber(_context.front().locals_num());

        _context.pop_front();
    }

    void ast_to_bytecode_visitor::store(AstVar const *var)
    {
        Instruction local = BC_INVALID;
        Instruction context = BC_INVALID;
        switch (var->type()) 
        {
            case VT_INT:
                local = BC_STOREIVAR;
                context = BC_STORECTXIVAR;
                break;
            case VT_DOUBLE:
                local = BC_STOREDVAR;
                context = BC_STORECTXDVAR;
                break;
            case VT_STRING:
                local = BC_STORESVAR;
                context = BC_STORECTXSVAR;
                break;
            default:
                throw ;
        }

        var_context var_info = v_context(var->name());
        if (var_info.context_id == _context.front().id())
        {
            bytecode()->addInsn(local);
        } 
        else 
        {
            bytecode()->addInsn(context);
            bytecode()->addUInt16(var_info.context_id);
        }
        bytecode()->addUInt16(var_info.var_id);

        set_tos_type(var->type());
    }

    void ast_to_bytecode_visitor::load(AstVar const *var)
    {
        Instruction local = BC_INVALID;
        Instruction context = BC_INVALID;
        switch (var->type()) 
        {
            case VT_INT:
                local = BC_LOADIVAR;
                context = BC_LOADCTXIVAR;
                break;
            case VT_DOUBLE:
                local = BC_LOADDVAR;
                context = BC_LOADCTXDVAR;
                break;
            case VT_STRING:
                local = BC_LOADSVAR;
                context = BC_LOADCTXSVAR;
                break;
            default:
                throw ;
        }

        var_context var_info = v_context(var->name());
        if (var_info.context_id == _context.front().id())
        {
            bytecode()->addInsn(local);
        } 
        else 
        {
            bytecode()->addInsn(context);
            bytecode()->addUInt16(var_info.context_id);
        }
        bytecode()->addUInt16(var_info.var_id);

        set_tos_type(var->type());
    }

    void ast_to_bytecode_visitor::visitBinaryOpNode(BinaryOpNode *node) 
    {
        switch (node->kind()) 
        {
            case tADD:
            case tSUB:
            case tDIV:
            case tMUL:
            case tMOD:
            {
                node->right()->visit(this);
                VarType right_ty = tos_type();
                node->left()->visit(this);
                VarType left_ty = tos_type();
                arithm_cast(left_ty, right_ty);

                if (tos_type() == VT_INT) 
                {
                    switch(node->kind()) 
                    {
                        case tADD:
                            bytecode()->addInsn(BC_IADD);
                            break;
                        case tMUL:
                            bytecode()->addInsn(BC_IMUL);
                            break;
                        case tSUB:
                            bytecode()->addInsn(BC_ISUB);
                            break;
                        case tDIV:
                            bytecode()->addInsn(BC_IDIV);
                            break;
                        case tMOD:
                            bytecode()->addInsn(BC_IMOD);
                            break;
                        default:
                            return;
                    }
                    set_tos_type(VT_INT);
                    return;
                }
                if (tos_type() == VT_DOUBLE) 
                {
                    switch(node->kind()) 
                    {
                        case tADD:
                            bytecode()->addInsn(BC_DADD);
                            break;
                        case tMUL:
                            bytecode()->addInsn(BC_DMUL);
                            break;
                        case tSUB:
                            bytecode()->addInsn(BC_DSUB);
                            break;
                        case tDIV:
                            bytecode()->addInsn(BC_DDIV);
                            break;
                        default:
                            return;
                    }
                    set_tos_type(VT_DOUBLE);
                }
                break;
            }
            case tAND:
            case tOR: 
            {
                Instruction insn = BC_INVALID;
                if (node->kind() == tAND) 
                {
                    insn = BC_ILOAD0;
                }
                if (node->kind() == tOR) 
                {
                    insn = BC_ILOAD1;
                }
                if (insn == BC_INVALID) 
                {
                    throw ;
                }

                node->left()->visit(this);
                tos_cast(VT_INT);

                bytecode()->addInsn(insn);
                Label right_lab = Label(bytecode());
                bytecode()->addBranch(BC_IFICMPE, right_lab);

                node->right()->visit(this);
                Label end_lab = Label(bytecode());
                bytecode()->addBranch(BC_JA, end_lab);

                bytecode()->bind(right_lab);
                bytecode()->addInsn(insn);

                bytecode()->bind(end_lab);

                tos_cast(VT_INT);
                set_tos_type(VT_INT);
                break;
            }
            case tAAND:
            case tAOR:
            case tAXOR:
            {
                node->right()->visit(this);
                tos_cast(VT_INT);
                node->left()->visit(this);
                tos_cast(VT_INT);
                switch (node->kind()) 
                {
                    case tAOR:
                        bytecode()->addInsn(BC_IAOR);
                        break;
                    case tAAND:
                        bytecode()->addInsn(BC_IAAND);
                        break;
                    case tAXOR:
                        bytecode()->addInsn(BC_IAXOR);
                        break;
                    default:
                        throw ;

                }

                set_tos_type(VT_INT);
                break;
            }
            case tEQ:
            case tNEQ:
            case tGE:
            case tLE:
            case tGT:
            case tLT:
            {
                Instruction insn = BC_INVALID;
                switch (node->kind()) 
                {
                    case tEQ:
                        insn = BC_IFICMPE;
                        break;
                    case tNEQ:
                        insn = BC_IFICMPNE;
                        break;
                    case tGE:
                        insn = BC_IFICMPGE;
                        break;
                    case tLE:
                        insn = BC_IFICMPLE;
                        break;
                    case tGT:
                        insn = BC_IFICMPG;
                        break;
                    case tLT:
                        insn = BC_IFICMPL;
                        break;
                    default:
                        throw ;
                }

                node->right()->visit(this);
                tos_cast(VT_INT);
                node->left()->visit(this);
                tos_cast(VT_INT);

                Label ll = Label(bytecode());
                bytecode()->addBranch(insn, ll);
                bytecode()->addInsn(BC_ILOAD0);
                Label rr = Label(bytecode());
                bytecode()->addBranch(BC_JA, rr);
                bytecode()->bind(ll);
                bytecode()->addInsn(BC_ILOAD1);
                bytecode()->bind(rr);

                set_tos_type(VT_INT);
                break;
            }
            default:
                return;
        }
    }


    void ast_to_bytecode_visitor::visitUnaryOpNode(UnaryOpNode *node) 
    {
        switch (node->kind()) 
        {
            case tNOT:
            {
                node->operand()->visit(this);
                if (tos_type() != VT_INT) {
                    throw ;
                }
                bytecode()->addInsn(BC_ILOAD0);
                Label ll = Label(bytecode());
                bytecode()->addBranch(BC_IFICMPE, ll);
                bytecode()->addInsn(BC_ILOAD0);
                Label rl = Label(bytecode());
                bytecode()->addBranch(BC_JA, rl);
                bytecode()->bind(ll);
                bytecode()->addInsn(BC_ILOAD1);
                bytecode()->bind(rl);

                set_tos_type(VT_INT);

                break;
            }
            case tSUB:
            {
                node->operand()->visit(this);
                switch (tos_type()) 
                {
                    case VT_STRING:
                        tos_cast(VT_INT);
                        bytecode()->addInsn(BC_INEG);
                        break;
                    case VT_INT:
                        bytecode()->addInsn(BC_INEG);
                        break;
                    case VT_DOUBLE:
                        bytecode()->addInsn(BC_DNEG);
                        break;
                    default:
                        throw ;
                }
                break;
            }
            default:
                return;
        }
    }


    void ast_to_bytecode_visitor::visitIntLiteralNode(IntLiteralNode *node)
    {
        bytecode()->addInsn(BC_ILOAD);
        bytecode()->addInt64(node->literal());
        set_tos_type(VT_INT);
    }


    void ast_to_bytecode_visitor::visitDoubleLiteralNode(DoubleLiteralNode *node)
    {
        bytecode()->addInsn(BC_DLOAD);
        bytecode()->addDouble(node->literal());
        set_tos_type(VT_DOUBLE);
    }


    void ast_to_bytecode_visitor::visitStringLiteralNode(StringLiteralNode *node)
    {
        bytecode()->addInsn(BC_SLOAD);
        bytecode()->addUInt16(_code->makeStringConstant(node->literal()));
        set_tos_type(VT_STRING);
    }


    void ast_to_bytecode_visitor::visitLoadNode(LoadNode *node)
    {
        load(node->var());
    }


    void ast_to_bytecode_visitor::visitStoreNode(StoreNode *node)
    {
        node->value()->visit(this);
        VarType var_type = node->var()->type();
        tos_cast(var_type);
        if (node->op() == tINCRSET)
        {
            load(node->var());
            Instruction inc_insn = (var_type == VT_INT) ? BC_IADD : BC_DADD;
            bytecode()->addInsn(inc_insn);
        }
        else if (node->op() == tDECRSET)
        {
            load(node->var());
            Instruction dec_insn = (var_type == VT_INT) ? BC_ISUB : BC_DSUB;
            bytecode()->addInsn(dec_insn);
        }

        store(node->var());
    }


    void ast_to_bytecode_visitor::visitIfNode(IfNode *node)
    {
        node->ifExpr()->visit(this);
        tos_cast(VT_INT);

        Label else_l = Label(bytecode());
        Label end_l = Label(bytecode());

        bytecode()->addInsn(BC_ILOAD0);
        bytecode()->addBranch(BC_IFICMPE, else_l);
        node->thenBlock()->visit(this);
        bytecode()->addBranch(BC_JA, end_l);

        bytecode()->bind(else_l);
        if (node->elseBlock())
            node->elseBlock()->visit(this);

        bytecode()->bind(end_l);

        set_tos_type(VT_VOID);
    }


    void ast_to_bytecode_visitor::visitForNode(ForNode *node)
    {
        BinaryOpNode* in_expr = node->inExpr()->asBinaryOpNode();
        if (!in_expr)
            throw ;

        in_expr->left()->visit(this);
        tos_cast(VT_INT);
        if (node->var()->type() != VT_INT) {
            throw ;
        }
        store(node->var());

        Label sl = Label(bytecode());
        Label el = Label(bytecode());

        bytecode()->bind(sl);

        in_expr->right()->visit(this);
        tos_cast(VT_INT);
        load(node->var());
        bytecode()->addBranch(BC_IFICMPG, el);

        node->body()->visit(this);

        load(node->var());
        bytecode()->addInsn(BC_ILOAD1);
        bytecode()->addInsn(BC_IADD);
        store(node->var());
        bytecode()->addBranch(BC_JA, sl);
        bytecode()->bind(el);

        set_tos_type(VT_VOID);
    }


    void ast_to_bytecode_visitor::visitWhileNode(WhileNode *node)
    {
        Label sl = Label(bytecode());
        Label el = Label(bytecode());

        bytecode()->bind(sl);
        node->whileExpr()->visit(this);
        tos_cast(VT_INT);
        bytecode()->addInsn(BC_ILOAD0);
        bytecode()->addBranch(BC_IFICMPE, el);
        node->loopBlock()->visit(this);
        bytecode()->addBranch(BC_JA, sl);
        bytecode()->bind(el);

        set_tos_type(VT_VOID);
    }


    void ast_to_bytecode_visitor::visitBlockNode(BlockNode *node)
    {

        for (Scope::VarIterator iter_var(node->scope()); iter_var.hasNext(); )
        {
            AstVar* var = iter_var.next();
            _context.front().add_var(var);
        }

        for (Scope::FunctionIterator iter_func(node->scope()); iter_func.hasNext(); )
        {
            AstFunction* ast_func = iter_func.next();

            BytecodeFunction* byte_func = (BytecodeFunction*) _code->functionByName(ast_func->name());
            if (!byte_func)
            {
                byte_func = new BytecodeFunction(ast_func);
                _code->addFunction(byte_func);
            }
            else
                throw ;
        }

        for (Scope::FunctionIterator iter_func = Scope::FunctionIterator(node->scope()); iter_func.hasNext(); )
            translate_func(iter_func.next());

        for (uint32_t i = 0; i < node->nodes(); ++i)
            node->nodeAt(i)->visit(this);

        set_tos_type(VT_VOID);
    }


    void ast_to_bytecode_visitor::visitFunctionNode(FunctionNode *node)
    {
        node->body()->visit(this);
        set_tos_type(VT_VOID);
    }


    void ast_to_bytecode_visitor::visitReturnNode(ReturnNode *node)
    {
        if (node->returnExpr())
        {
            node->returnExpr()->visit(this);
            tos_cast(_context.front().byte_func()->returnType());
        }
        bytecode()->addInsn(BC_RETURN);

        set_tos_type(_context.front().byte_func()->returnType());
    }


    void ast_to_bytecode_visitor::visitNativeCallNode(NativeCallNode *node)
    {

        void *code = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
        if (!code)
        {
            throw ;
        }
        uint16_t func_id = _code->makeNativeFunction(node->nativeName(), node->nativeSignature(), code);
        bytecode()->addInsn(BC_CALLNATIVE);
        bytecode()->addUInt16(func_id);

        set_tos_type(node->nativeSignature()[0].first);
    }


    void ast_to_bytecode_visitor::visitCallNode(CallNode *node)
    {
        BytecodeFunction* byte_func = (BytecodeFunction*) _code->functionByName(node->name());

        for (int i = (int)node->parametersNumber() - 1; i >= 0; --i)
        {
            node->parameterAt(i)->visit(this);
            tos_cast(byte_func->parameterType(i));
        }

        bytecode()->addInsn(BC_CALL);
        bytecode()->addUInt16(byte_func->id());

        set_tos_type(byte_func->returnType());
    }


    void ast_to_bytecode_visitor::visitPrintNode(PrintNode *node)
    {
        for (uint32_t i = 0; i < node->operands(); ++i)
        {
            node->operandAt(i)->visit(this);
            Instruction insn = BC_INVALID;
            switch (tos_type())
            {
                case VT_INT:
                    insn = BC_IPRINT;
                    break;
                case VT_DOUBLE:
                    insn = BC_DPRINT;
                    break;
                case VT_STRING:
                    insn = BC_SPRINT;
                    break;
                default:
                    return;
            }
            bytecode()->addInsn(insn);
            set_tos_type(VT_VOID);
        }
    }
    
    void ast_to_bytecode_visitor::tos_cast(VarType type)
    {
        if (type == VT_DOUBLE)
        {
            switch (tos_type())
            {
                case VT_DOUBLE:
                    break;
                case VT_INT:
                    bytecode()->addInsn(BC_I2D);
                    break;
                default:
                    throw ;
            }
            set_tos_type(VT_DOUBLE);
            return;
        }

        if (type == VT_INT)
        {
            switch (tos_type())
            {
                case VT_INT:
                    break;
                case VT_DOUBLE:
                    bytecode()->addInsn(BC_D2I);
                    break;
                default:
                    throw ;
            }
            set_tos_type(VT_INT);
            return;
        }

        if (type == VT_STRING && tos_type() == VT_STRING)
        {
            return;
        }
        throw ;
    }


    void ast_to_bytecode_visitor::arithm_cast(VarType l_type, VarType r_type)
    {
        if (l_type == VT_DOUBLE)
        {
            switch (r_type)
            {
                case VT_DOUBLE:
                    return;
                case VT_INT:
                    bytecode()->addInsn(BC_SWAP);
                    bytecode()->addInsn(BC_I2D);
                    bytecode()->addInsn(BC_SWAP);
                    return;
                default:
                    break;
            }
        }

        if (l_type == VT_INT)
        {
            switch (r_type)
            {
                case VT_INT:
                    return;
                case VT_DOUBLE:
                    bytecode()->addInsn(BC_I2D);
                    set_tos_type(VT_DOUBLE);
                    return;
                default:
                    break;
            }
        }

        throw ;
    }


} // mathvm
