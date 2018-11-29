#include <cassert>
#include "../../../vm/parser.h"
#include "typer.hpp"
#include "bytecode_interpreter.hpp"
#include <dlfcn.h>

#define TODO assert(0)

namespace mathvm {

class BytecodeVisitor : public AstVisitor {
    private:

        void generateComparison(TokenKind kind) {
            Label label1(bytecode);
            Label label2(bytecode);
            switch (kind) {
                case tEQ:    // "==", 9)
                    bytecode->addBranch(BC_IFICMPE, label1);
                    break;
                case tNEQ:   // "!=", 9)
                    bytecode->addBranch(BC_IFICMPNE, label1);
                    break;
                case tGT:    // ">", 10)
                    bytecode->addBranch(BC_IFICMPG, label1);
                    break;
                case tGE:    // ">=", 10)
                    bytecode->addBranch(BC_IFICMPGE, label1);
                    break;
                case tLT:    // "<", 10)
                    bytecode->addBranch(BC_IFICMPL, label1);
                    break;
                case tLE:    // "<=", 10)
                    bytecode->addBranch(BC_IFICMPLE, label1);
                    break;
                default:
                    assert(0);
            }
            bytecode->addInsn(BC_ILOAD0);
            bytecode->addBranch(BC_JA, label2);
            bytecode->bind(label1);
            bytecode->addInsn(BC_ILOAD1);
            bytecode->bind(label2);
        }

#define VAR_OP(OP, N) \
    switch (var->type()) { \
        case DOUBLE: bytecode->addInsn(BC_##OP##DVAR##N); break;\
        case INT   : bytecode->addInsn(BC_##OP##IVAR##N); break;\
        case STRING: bytecode->addInsn(BC_##OP##SVAR##N); break;\
        default: assert(0); break; }
        void storeVar(const AstVar *var) {
            uint16_t id = GET_VAR_ID(var);
            uint16_t ctx = GET_CTX(var);
            if (GET_FN_ID(currentFunction) == ctx) {
                switch (id) {
                    case 0:
                        VAR_OP(STORE, 0);
                        break;
                    case 1:
                        VAR_OP(STORE, 1);
                        break;
                    case 2:
                        VAR_OP(STORE, 2);
                        break;
                    case 3:
                        VAR_OP(STORE, 3);
                        break;
                    default:
                        VAR_OP(STORE,);
                        bytecode->addInt16(id);
                }
            } else {
                VAR_OP(STORECTX, );
                bytecode->addInt16(ctx);
                bytecode->addInt16(id);
            }
        }

        void loadVar(const AstVar *var) {
            uint16_t id = GET_VAR_ID(var);
            uint16_t ctx = GET_CTX(var);
            if (GET_FN_ID(currentFunction) == ctx) {
                switch (id) {
                    case 0:
                        VAR_OP(LOAD, 0);
                        break;
                    case 1:
                        VAR_OP(LOAD, 1);
                        break;
                    case 2:
                        VAR_OP(LOAD, 2);
                        break;
                    case 3:
                        VAR_OP(LOAD, 3);
                        break;
                    default:
                        VAR_OP(LOAD,);
                        bytecode->addInt16(id);
                }
            } else {
                VAR_OP(LOADCTX, );
                bytecode->addInt16(ctx);
                bytecode->addInt16(id);
            }
        }
#undef VAR_OP

        void fixType(Type dst, Type src) {
            if (dst == src) {
                return;
            }
            switch (dst) {
                case INT:
                    switch (src) {
                        case DOUBLE:
                            bytecode->addInsn(BC_D2I);
                            break;
                        case STRING:
                            bytecode->addInsn(BC_S2I);
                            break;
                        case BOOL:
                        case INT:
                            break;
                        default:
                            assert(0);
                    }
                    break;
                case DOUBLE:
                    switch (src) {
                        case STRING:
                            bytecode->addInsn(BC_S2I);
                            bytecode->addInsn(BC_I2D);
                            break;
                        case INT:
                        case BOOL:
                            bytecode->addInsn(BC_I2D);
                            break;
                        case DOUBLE:
                            break;
                        default:
                            assert(0);
                    }
                    break;
                case BOOL:
                    switch (src) {
                        case STRING:
                            bytecode->addInsn(BC_S2I);
                        case BOOL:
                        case INT:
                            bytecode->addInsn(BC_ILOAD0);
                            generateComparison(tNEQ);
                            break;
                        case DOUBLE:
                            assert(0);
                            break;
                        default:
                            assert(0);
                    }
                    break;
                default:
                    assert(0);
            }
        }

        Bytecode *bytecode;
        AstFunction *currentFunction;
        Code *code;
        Scope *currentScope;

    public:

        BytecodeVisitor(Code *code): currentFunction(nullptr), code(code)
        { }

#define ADD_D_I_INST(inst) bytecode->addInsn(argtp == DOUBLE ? BC_D##inst : BC_I##inst);
        void visitBinaryOpNode(BinaryOpNode* node) {
            if (node->kind() == tOR) {
                Label ok(bytecode), end(bytecode);
                node->left()->visit(this);
                bytecode->addInsn(BC_ILOAD0);
                bytecode->addBranch(BC_IFICMPNE, ok);
                node->right()->visit(this);
                bytecode->addInsn(BC_ILOAD0);
                bytecode->addBranch(BC_IFICMPNE, ok);
                bytecode->addInsn(BC_ILOAD0);
                bytecode->addBranch(BC_JA, end);
                bytecode->bind(ok);
                bytecode->addInsn(BC_ILOAD1);
                bytecode->bind(end);
                return;
            } else if (node->kind() == tAND) {
                Label ok(bytecode), end(bytecode);
                node->left()->visit(this);
                bytecode->addInsn(BC_ILOAD0);
                bytecode->addBranch(BC_IFICMPE, ok);
                node->right()->visit(this);
                bytecode->addInsn(BC_ILOAD0);
                bytecode->addBranch(BC_IFICMPE, ok);
                bytecode->addInsn(BC_ILOAD1);
                bytecode->addBranch(BC_JA, end);
                bytecode->bind(ok);
                bytecode->addInsn(BC_ILOAD0);
                bytecode->bind(end);
                return;
            }
            node->left()->visit(this);
            node->right()->visit(this);
            Type argtp = GET_TYPE(node->right());
            Type type = argtp;
            Type thisType = GET_TYPE(node);
            switch (node->kind()) {
                case tADD:   // "+", 12)
                    ADD_D_I_INST(ADD);
                    break;
                case tSUB:   // "-", 12)
                    bytecode->addInsn(BC_SWAP);
                    ADD_D_I_INST(SUB);
                    break;
                case tMUL:   // "*", 13)
                    ADD_D_I_INST(MUL);
                    break;
                case tDIV:   // "/", 13)
                    bytecode->addInsn(BC_SWAP);
                    ADD_D_I_INST(DIV);
                    break;
                case tMOD:   // "%", 13)
                    bytecode->addInsn(BC_SWAP);
                    bytecode->addInsn(BC_IMOD);
                    break;
                case tEQ:    // "==", 9)
                case tNEQ:   // "!=", 9)
                case tGT:    // ">", 10)
                case tGE:    // ">=", 10)
                case tLT:    // "<", 10)
                case tLE:    // "<=", 10)
                    bytecode->addInsn(BC_SWAP);
                    if (argtp == INT || argtp == BOOL) {
                        generateComparison(node->kind());
                    } else if (argtp == DOUBLE) {
                        ADD_D_I_INST(CMP);
                        bytecode->addInsn(BC_ILOAD0);
                        bytecode->addInsn(BC_SWAP);
                        generateComparison(node->kind());
                    }
                    type = BOOL;
                    break;
                case tOR:    // "||", 4)
                    if (argtp == BOOL) {
                        bytecode->addInsn(BC_IAOR);
                    } else {
                    }
                    type = BOOL;
                    break;
                case tAND:   // "&&", 5)
                    if (argtp == BOOL) {
                        bytecode->addInsn(BC_IAAND);
                    } else {
                    }
                    type = BOOL;
                    break;
                case tAOR:   // "|", 4)
                    bytecode->addInsn(BC_IAOR);
                    break;
                case tAAND:  // "&", 5)
                    bytecode->addInsn(BC_IAAND);
                    break;
                case tAXOR:  // "^", 5)
                    bytecode->addInsn(BC_IAXOR);
                    break;
                default:
                    assert(0);
                    break;
            }
            fixType(thisType, type);
        }

        void visitUnaryOpNode(UnaryOpNode* node) {
            node->operand()->visit(this);
            Type argtp = GET_TYPE(node->operand());
            switch (node->kind()) {
                case tSUB:
                    ADD_D_I_INST(NEG);
                    break;
                case tNOT:
                    switch (argtp) {
                        case BOOL:
                            bytecode->addInsn(BC_ILOAD1);
                            bytecode->addInsn(BC_IAXOR);
                            break;
                        case INT:
                            bytecode->addInsn(BC_ILOAD0);
                            generateComparison(tEQ);
                            break;
                        default:
                            assert(0);
                    }
                    break;
                default:
                    assert(0);
                    break;
            }
        }

        void visitStringLiteralNode(StringLiteralNode* node) {
            const string& lit = node->literal();
            if (lit != "") {
                uint16_t id = code->makeStringConstant(lit);
                bytecode->addInsn(BC_SLOAD);
                bytecode->addUInt16(id);
            } else {
                bytecode->addInsn(BC_SLOAD0);
            }
        }

        void visitDoubleLiteralNode(DoubleLiteralNode* node) {
            double c = node->literal();
            if (c == 0) {
                bytecode->addInsn(BC_DLOAD0);
            } else if (c == 1) {
                bytecode->addInsn(BC_DLOAD1);
            } else if (c == -1) {
                bytecode->addInsn(BC_DLOADM1);
            } else {
                bytecode->addInsn(BC_DLOAD);
                bytecode->addDouble(c);
            }
        }

        void visitIntLiteralNode(IntLiteralNode* node) {
            int64_t c = node->literal();
            switch (c) {
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
                    bytecode->addInt64(c);
                    break;
            }
        }

        void visitLoadNode(LoadNode* node) {
            loadVar(node->var());
            fixType(GET_TYPE(node), (Type)(int)(node->var()->type()));
        }

        void visitStoreNode(StoreNode* node) {
            node->value()->visit(this);
            Type argtp = (Type)(int)node->var()->type();
            switch (node->op()) {
                case tASSIGN:
                    break;
                case tINCRSET:
                    loadVar(node->var());
                    ADD_D_I_INST(ADD);
                    break;
                case tDECRSET:
                    loadVar(node->var());
                    ADD_D_I_INST(SUB);
                    break;
                default:
                    assert(0);
            }
            storeVar(node->var());
        }
#undef ADD_D_I_INST

        void visitForNode(ForNode* node) {
            BinaryOpNode *inExpr = dynamic_cast<BinaryOpNode*>(node->inExpr());
            assert(inExpr);
            assert(inExpr->kind() == tRANGE);

            Label label1(bytecode), label2(bytecode);
            const AstVar *var = node->var();

            inExpr->left()->visit(this);
            storeVar(var);
            bytecode->addBranch(BC_JA, label1);

            bytecode->bind(label2);
            node->body()->visit(this);
            loadVar(var);
            bytecode->addInsn(BC_ILOAD1);
            bytecode->addInsn(BC_IADD);
            storeVar(var);
            bytecode->bind(label1);
            inExpr->right()->visit(this);
            loadVar(var);
            bytecode->addBranch(BC_IFICMPLE, label2);
        }

        void visitWhileNode(WhileNode* node) {
            Label label1(bytecode), label2(bytecode);
            bytecode->addBranch(BC_JA, label1);
            bytecode->bind(label2);
            node->loopBlock()->visit(this);
            bytecode->bind(label1);
            node->whileExpr()->visit(this);
            bytecode->addInsn(BC_ILOAD0);
            bytecode->addBranch(BC_IFICMPNE, label2);
        }

        void visitIfNode(IfNode* node) {
            Label label1(bytecode);
            node->ifExpr()->visit(this);
            bytecode->addInsn(BC_ILOAD0);
            bytecode->addBranch(BC_IFICMPE, label1);
            node->thenBlock()->visit(this);
            BlockNode* el = node->elseBlock();
            if (el) {
                Label label2(bytecode);
                bytecode->addBranch(BC_JA, label2);
                bytecode->bind(label1);
                el->visit(this);
                bytecode->bind(label2);
            } else {
                bytecode->bind(label1);
            }
        }

        void enterFunction(AstFunction *function) {
            AstFunction *oldCurrentFunction = currentFunction;
            currentFunction = function;
            bytecode = ((BytecodeFunction*)currentFunction->info())->bytecode();
            function->node()->visit(this);
            if (function->name() == "<top>") {
                bytecode->addInsn(BC_STOP);
            }
            currentFunction = oldCurrentFunction;
            if (currentFunction) {
                bytecode =
                    ((BytecodeFunction*)currentFunction->info())->bytecode();
            }
        }

        void visitBlockNode(BlockNode* node) {
            Scope *oldScope = currentScope;
            currentScope = node->scope();

            Scope::FunctionIterator fit(node->scope());
            while (fit.hasNext()) {
                AstFunction *next = fit.next();
                enterFunction(next);
            }

            for (uint32_t i = 0; i < node->nodes(); ++i) {
                AstNode *cur = node->nodeAt(i);
                cur->visit(this);
                if (GET_TYPE(cur) != NONE) {
                    bytecode->addInsn(BC_POP);
                }
            }

            currentScope = oldScope;
        }

        void visitFunctionNode(FunctionNode* node) {
            BlockNode* body = node->body();
            if (body->nodes() == 2) {
                NativeCallNode* native = dynamic_cast<NativeCallNode*>
                    (body->nodeAt(0));
                if (native) {
                    void *fnAddr = dlsym(RTLD_DEFAULT,
                        native->nativeName().c_str());
                    assert(fnAddr);
                    uint16_t fnId = code->makeNativeFunction(node->name(),
                        native->nativeSignature(),
                        fnAddr);
                    bytecode->addInsn(BC_CALLNATIVE);
                    bytecode->addInt16(fnId);
                    bytecode->addInsn(BC_RETURN);
                    return;
                }
            }
            Scope *paramScope = node->body()->scope()->parent();
            for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
                AstVar *var = paramScope->lookupVariable(
                    node->parameterName(node->parametersNumber()-i-1));
                storeVar(var);
            }
            body->visit(this);
        }

        void visitReturnNode(ReturnNode* node) {
            AstNode *expr = node->returnExpr();
            if (expr) {
                expr->visit(this);
            }
            if (currentFunction->name() == "<top>") {
                bytecode->addInsn(BC_STOP);
            } else {
                bytecode->addInsn(BC_RETURN);
            }
        }

        void visitNativeCallNode(NativeCallNode* node) {
            assert(0);
        }

        void visitCallNode(CallNode* node) {
            for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
                node->parameterAt(i)->visit(this);
            }
            bytecode->addInsn(BC_CALL);
            AstFunction *fn = currentScope->lookupFunction(node->name());
            assert(fn);
            bytecode->addInt16(GET_FN_ID(fn));
            fixType(GET_TYPE(node), (Type)(int)fn->returnType());
        }

        void visitPrintNode(PrintNode* node) {
            for (uint32_t i = 0; i < node->operands(); ++i) {
                AstNode* child = node->operandAt(i);
                child->visit(this);
                switch (GET_TYPE(child)) {
                    case INT:
                    case BOOL:
                        bytecode->addInsn(BC_IPRINT);
                        break;
                    case DOUBLE:
                        bytecode->addInsn(BC_DPRINT);
                        break;
                    case STRING:
                        bytecode->addInsn(BC_SPRINT);
                        break;
                    case NONE:
                        assert(0);
                        break;
                }
            }
        }

};

Status* BytecodeTranslatorImpl::translate(const string& program, Code* *code) {
    Parser parser;
    Status *status = parser.parseProgram(program);
    if (!status->isOk()) {
        return status;
    }
    AstFunction *node = parser.top();

    *code = new MathvmCode();
    assignTypes(node, *code);

    BytecodeVisitor visitor(*code);
    visitor.enterFunction(node);

    return status;
}

}
