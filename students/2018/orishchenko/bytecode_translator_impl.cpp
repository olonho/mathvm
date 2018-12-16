#include <dlfcn.h>
#include "bytecode_translator_impl.h"


namespace mathvm {

    Status *BytecodeTranslatorImpl::translate(const string &program, Code **code) {
        Parser parser;
        Status *status = parser.parseProgram(program);
        if (status->isError())
            return status;

        InterpreterCode *interpreter = new InterpreterCode();
        try {
            BytecodeVisitor exec(parser.top(), interpreter);
            *code = exec.code;
        } catch (...) {
            return Status::Error("error");
        }
        return Status::Ok();
    }

    BytecodeVisitor::BytecodeVisitor(AstFunction *top, InterpreterCode *code) : code(code) {
        BytecodeFunction *bytecodeFun = new BytecodeFunction(top);
        code->addFunction(bytecodeFun);
        context = new VisitorCtx(0, nullptr);
        code->add_ctx(context, bytecodeFun);
        top->node()->visit(this);
    }

    BytecodeVisitor::~BytecodeVisitor() {}

    void BytecodeVisitor::visitForNode(ForNode *node) {

        Label start(bytecode.top());
        Label end(bytecode.top());

        node->inExpr()->asBinaryOpNode()->left()->visit(this);
        save(node->var()->type(), node->var()->name());
        bytecode.top()->bind(start);
        node->inExpr()->asBinaryOpNode()->right()->visit(this);
        load(node->var()->type(), node->var()->name());
        bytecode.top()->addBranch(BC_IFICMPG, end);
        node->body()->visit(this);
        load(node->var()->type(), node->var()->name());
        bytecode.top()->addInsn(BC_ILOAD1);
        bytecode.top()->addInsn(BC_IADD);
        save(node->var()->type(), node->var()->name());
        bytecode.top()->addBranch(BC_JA, start);
        bytecode.top()->bind(end);
    }

    void BytecodeVisitor::visitPrintNode(PrintNode *node) {
        for (uint32_t i = 0; i < node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            VarType type = pop_type();
            if (type == VT_STRING) {
                bytecode.top()->addInsn(BC_SPRINT);
            }
            if (type == VT_INT) {
                bytecode.top()->addInsn(BC_IPRINT);
            }
            if (type == VT_DOUBLE) {
                bytecode.top()->addInsn(BC_DPRINT);
            }
        }
    }

    void BytecodeVisitor::visitLoadNode(LoadNode *node) {
        load(node->var()->type(), node->var()->name());
    }

    void BytecodeVisitor::visitIfNode(IfNode *node) {
        Label start(bytecode.top());
        Label end(bytecode.top());

        node->ifExpr()->visit(this);
        bytecode.top()->addInsn(BC_ILOAD0);
        bytecode.top()->addBranch(BC_IFICMPE, start);
        node->thenBlock()->visit(this);

        bytecode.top()->addBranch(BC_JA, end);
        bytecode.top()->bind(start);

        if (node->elseBlock())
            node->elseBlock()->visit(this);
        bytecode.top()->bind(end);
    }

    void BytecodeVisitor::visitCallNode(CallNode *node) {
        auto *f = (BytecodeFunction *) code->functionByName(node->name());

        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {

            node->parameterAt(i)->visit(this);
            auto type = f->parameterType(i);
            cast_top(type);
        }

        bytecode.top()->addInsn(BC_CALL);
        bytecode.top()->addInt16(f->id());
        VarType type1 = f->returnType();
        typesStack.push(type1);
    }

    void BytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
        bytecode.top()->addInsn(BC_DLOAD);
        bytecode.top()->addDouble(node->literal());
        typesStack.push(VT_DOUBLE);
    }

    void BytecodeVisitor::visitStoreNode(StoreNode *node) {
        node->visitChildren(this);
        VarType type = node->var()->type();
        cast_top(type);
        if (node->op() == tINCRSET || node->op() == tDECRSET) {
            load(typesStack.top(), node->var()->name());
            Instruction insn = BC_INVALID;
            if (node->op() == tDECRSET)
                bytecode.top()->addInsn(BC_SWAP);
            if (typesStack.top() == VT_INT && node->op() == tINCRSET) {
                insn = BC_IADD;
            } else if (type == VT_DOUBLE && node->op() == tINCRSET) {
                insn = BC_DADD;
            } else if (type == VT_INT && node->op() == tDECRSET) {
                insn = BC_ISUB;
            } else if (type == VT_DOUBLE && node->op() == tDECRSET) {
                insn = BC_DSUB;
            }
            bytecode.top()->addInsn(insn);
        }
        save(typesStack.top(), node->var()->name());
        pop_type();
    }

    void BytecodeVisitor::visitStringLiteralNode(StringLiteralNode *node) {
        bytecode.top()->addInsn(BC_SLOAD);
        bytecode.top()->addUInt16(code->makeStringConstant(node->literal()));
        typesStack.push(VT_STRING);
    }

    void BytecodeVisitor::visitWhileNode(WhileNode *node) {
        Label start(bytecode.top());
        Label end(bytecode.top());

        bytecode.top()->bind(start);
        node->whileExpr()->visit(this);
        bytecode.top()->addInsn(BC_ILOAD0);
        bytecode.top()->addBranch(BC_IFICMPE, end);
        node->loopBlock()->visit(this);
        bytecode.top()->addBranch(BC_JA, start);
        bytecode.top()->bind(end);
    }

    void BytecodeVisitor::visitIntLiteralNode(IntLiteralNode *node) {
        bytecode.top()->addInsn(BC_ILOAD);
        bytecode.top()->addInt64(node->literal());
        typesStack.push(VT_INT);
    }

    void BytecodeVisitor::visitBlockNode(BlockNode *node) {
        if (!inside_function) {
            auto newContext = new VisitorCtx(static_cast<uint16_t>(context->address + 1), context);
            code->add_ctx(newContext, cur_function);
            context = newContext;
        } else
            inside_function = false;
        Scope::VarIterator varIt(node->scope());
        while (varIt.hasNext()) {
            context->variables.insert(make_pair(varIt.next()->name(), context->variables.size()));
        }
        Scope::FunctionIterator funIt(node->scope());
        while (funIt.hasNext()) {
            BytecodeFunction *bytecodeFun = new BytecodeFunction(funIt.next());
            code->addFunction(bytecodeFun);
        }
        Scope::FunctionIterator fs(node->scope());
        while (fs.hasNext()) {
            fs.next()->node()->visit(this);
        }
        node->visitChildren(this);
        context = context->parent;
    }

    void BytecodeVisitor::visitBinaryOpNode(BinaryOpNode *node) {

        auto kind = node->kind();

        if (kind == tOR || kind == tAND) {
            Label start(bytecode.top());
            Label end(bytecode.top());
            node->left()->visit(this);
            bytecode.top()->addInsn((kind == tOR) ? BC_ILOAD1 : BC_ILOAD0);
            bytecode.top()->addBranch(BC_IFICMPE, start);
            node->right()->visit(this);
            bytecode.top()->addBranch(BC_JA, end);
            bytecode.top()->bind(start);
            bytecode.top()->addInsn((kind == tOR) ? BC_ILOAD1 : BC_ILOAD0);
            bytecode.top()->bind(end);
            return;
        }

        node->left()->visit(this);
        node->right()->visit(this);
        convert();


        if (kind == tADD || kind == tSUB || kind == tMUL ||
            kind == tDIV || kind == tMOD) {
            calcFunction(node);
            return;
        }


        if (kind == tAOR) {
            bytecode.top()->addInsn(BC_IAOR);
        }
        if (kind == tAAND) {
            bytecode.top()->addInsn(BC_IAAND);
        }
        if (kind == tAXOR) {
            bytecode.top()->addInsn(BC_IAXOR);
        }


        if (kind == tEQ || kind == tNEQ || kind == tGT ||
            kind == tGE || kind == tLT || kind == tLE)
            comparingFunction(node);

    }

    void BytecodeVisitor::visitUnaryOpNode(UnaryOpNode *node) {
        node->visitChildren(this);
        if (node->kind() == tNOT) {
            bytecode.top()->addInsn(BC_ILOADM1);
            bytecode.top()->addInsn(BC_IMUL);
            bytecode.top()->addInsn(BC_ILOAD1);
            bytecode.top()->addInsn(BC_IADD);
        }
        if (node->kind() == tSUB) {
            auto instr = (typesStack.top() == VT_INT) ? BC_INEG : BC_DNEG;
            bytecode.top()->addInsn(instr);
        }
    }

    void BytecodeVisitor::visitReturnNode(ReturnNode *node) {
        if (node->returnExpr() == nullptr) {
            typesStack.push(VT_VOID);
        } else
            node->returnExpr()->visit(this);

        bytecode.top()->addInsn(BC_RETURN);
    }

    void BytecodeVisitor::visitFunctionNode(FunctionNode *node) {
        inside_function = true;
        auto fun = (BytecodeFunction *) code->functionByName(node->name());
        auto old_function = cur_function;
        cur_function = fun;
        bytecode.push(fun->bytecode());

        auto newContext = new VisitorCtx(static_cast<uint16_t>(context->address + 1), context);
        code->add_ctx(newContext, fun);
        context = newContext;

        for (uint32_t i = node->parametersNumber(); i > 0; --i) {
            context->addVariable(node->parameterName(i - 1));
            save(node->parameterType(i - 1), node->parameterName(i - 1));
        }
        node->body()->visit(this);

        if (node->name() == "<top>") {
            bytecode.top()->addInsn(BC_STOP);
            return;
        }
        bytecode.top()->addInsn(BC_RETURN);
        bytecode.pop();
        cur_function = old_function;

        VarType type = node->returnType();
        typesStack.push(type);
    }

    void BytecodeVisitor::comparingFunction(BinaryOpNode *node) {

        Label start(bytecode.top());
        Label end(bytecode.top());

        if (typesStack.top() == VT_DOUBLE) {
            bytecode.top()->addInsn(BC_DCMP);
            bytecode.top()->addInsn(BC_LOADIVAR0);
            bytecode.top()->addInsn(BC_SWAP);
        }

        switch (node->kind()) {
            case tEQ : {
                bytecode.top()->addBranch(BC_IFICMPE, start);
            }
                break;
            case tNEQ : {
                bytecode.top()->addBranch(BC_IFICMPNE, start);
            }
                break;
            case tGT : {
                bytecode.top()->addBranch(BC_IFICMPL, start);
            }
                break;
            case tGE : {
                bytecode.top()->addBranch(BC_IFICMPLE, start);
            }
                break;
            case tLT : {
                bytecode.top()->addBranch(BC_IFICMPG, start);
            }
                break;
            case tLE : {
                bytecode.top()->addBranch(BC_IFICMPGE, start);
            }
                break;
            default:
                break;
        }
        bytecode.top()->addInsn(BC_ILOAD0);
        bytecode.top()->addBranch(BC_JA, end);
        bytecode.top()->bind(start);

        bytecode.top()->addInsn(BC_ILOAD1);
        bytecode.top()->bind(end);
        typesStack.push(VT_INT);
    }

    void BytecodeVisitor::visitNativeCallNode(NativeCallNode *node) {}

    void BytecodeVisitor::convert() {
        auto left = pop_type();
        auto right = pop_type();

        if (left == VT_STRING && right == VT_STRING) {
            typesStack.push(VT_STRING);
        }
        if (left == VT_DOUBLE && right == VT_DOUBLE) {
            typesStack.push(VT_DOUBLE);
        }
        if (left == VT_DOUBLE && right == VT_INT) {
            typesStack.push(VT_DOUBLE);
            bytecode.top()->addInsn(BC_SWAP);
            bytecode.top()->addInsn(BC_I2D);
            bytecode.top()->addInsn(BC_SWAP);
        }
        if (left == VT_INT && right == VT_INT) {
            typesStack.push(VT_INT);
        }
        if (left == VT_INT && right == VT_DOUBLE) {
            typesStack.push(VT_DOUBLE);
            bytecode.top()->addInsn(BC_I2D);
        }
        typesStack.push(typesStack.top());
    }

    VarType BytecodeVisitor::pop_type() {
        auto top = typesStack.top();
        typesStack.pop();
        return top;
    }

    void BytecodeVisitor::cast_top(VarType type) {
        auto topType = typesStack.top();

        if (topType == VT_STRING && type == VT_DOUBLE) {
            bytecode.top()->addInsn(BC_S2I);
            bytecode.top()->addInsn(BC_I2D);
        }
        if (topType == VT_INT && type == VT_DOUBLE) {
            bytecode.top()->addInsn(BC_I2D);
        }
        if (topType == VT_STRING && type == VT_INT) {
            bytecode.top()->addInsn(BC_S2I);
        }
        if (topType == VT_DOUBLE && type == VT_INT) {
            bytecode.top()->addInsn(BC_D2I);
        }
    }

    void BytecodeVisitor::calcFunction(BinaryOpNode *node) {

        auto top = typesStack.top();
        switch (node->kind()) {
            case tADD: {
                (top == VT_DOUBLE) ?
                bytecode.top()->addInsn(BC_DADD) :
                bytecode.top()->addInsn(BC_IADD);
                break;
                case tSUB:
                    (top == VT_DOUBLE) ?
                    bytecode.top()->addInsn(BC_DSUB) :
                    bytecode.top()->addInsn(BC_ISUB);
                break;
                case tMUL:
                    (top == VT_DOUBLE) ?
                    bytecode.top()->addInsn(BC_DMUL) :
                    bytecode.top()->addInsn(BC_IMUL);
                break;
                case tDIV:
                    (top == VT_DOUBLE) ?
                    bytecode.top()->addInsn(BC_DDIV) :
                    bytecode.top()->addInsn(BC_IDIV);
                break;
                case tMOD:
                    if (top != VT_DOUBLE) {
                        bytecode.top()->addInsn(BC_IMOD);
                    }
                break;
                default:
                    break;
            }
        }
    }

    void BytecodeVisitor::load(VarType type, const string &name) {
        if (type == VT_STRING) {
            bytecode.top()->addInsn(BC_LOADCTXSVAR);
        }
        if (type == VT_INT) {
            bytecode.top()->addInsn(BC_LOADCTXIVAR);
        }
        if (type == VT_DOUBLE) {
            bytecode.top()->addInsn(BC_LOADCTXDVAR);
        }
        if (type == VT_VOID) {
            bytecode.top()->addInsn(BC_ILOAD0);
        }
        if (type != VT_VOID) {
            auto ctx = context;
            while (ctx->variables.find(name) == ctx->variables.end()) {
                ctx = ctx->parent;
                if (ctx == nullptr)
                    break;
            }
            if (ctx == nullptr)
                return;
            bytecode.top()->addInt16(ctx->address);
            bytecode.top()->addInt16(ctx->variables[name]);
        }

        typesStack.push(type);
    }

    void BytecodeVisitor::save(VarType type, const string &name) {
        if (type == VT_STRING) {
            bytecode.top()->addInsn(BC_STORECTXSVAR);
        }
        if (type == VT_INT) {
            bytecode.top()->addInsn(BC_STORECTXIVAR);
        }
        if (type == VT_DOUBLE) {
            bytecode.top()->addInsn(BC_STORECTXDVAR);
        }
        if (type != VT_VOID) {
            auto ctx = context;
            while (ctx->variables.find(name) == ctx->variables.end()) {
                ctx = ctx->parent;
                if (ctx == nullptr)
                    break;
            }
            if (ctx == nullptr)
                return;
            bytecode.top()->addInt16(ctx->address);
            bytecode.top()->addInt16(ctx->variables[name]);
        }
    }

}