#include "typer.hpp"
#include <mathvm.h>

#define SET_TYPE(node, type) (((AstNode*)node)->setInfo((void*)type))
#define SET_CTX(var, ctx) (((AstVar*)var)->setInfo((void*)(AstFunction*)ctx))

namespace mathvm {

class ExprTyperVisitor : public AstVisitor {

  private:

    Code *code;
    Scope *currentScope = nullptr;
    AstFunction *currentFunction = nullptr;
    uint16_t varCnt = 0;
    uint16_t funId  = 0;

    Type commonType(Type a, Type b) {
        if (a == b) {
            return a;
        } else if (a == NONE || b == NONE) {
            return NONE;
        } else if (a == INT) {
            return b == DOUBLE ? DOUBLE : INT;
        } else if (a == DOUBLE) {
            return DOUBLE;
        } else if (a == STRING) {
            return b == DOUBLE ? DOUBLE : INT;
        } else {
            return b;
        }
    }

    void accessVar(const AstVar *var) {
    }

    void registerVar(AstVar *var) {
        var->setInfo((void*)((funId << 16) | varCnt));
        ++varCnt;
    }

  public:

    ExprTyperVisitor(Code* code): code(code) { }

    void enterFunction(AstFunction* node) {

        AstFunction *oldCurrentFunction = currentFunction;
        int oldFunId = funId;
        uint16_t oldVarCnt = varCnt;

        currentFunction = node;
        varCnt = 0;

        BytecodeFunction *descriptor = new BytecodeFunction(node);
        funId = code->addFunction(descriptor);
        node->setInfo(descriptor);

        node->node()->visit(this);
        descriptor->setLocalsNumber(varCnt);

        varCnt = oldVarCnt;
        currentFunction = oldCurrentFunction;
        funId = oldFunId;
    }

    void visitBinaryOpNode(BinaryOpNode* node) {
        node->right()->visit(this);
        node->left()->visit(this);
        Type a, b;
        a = GET_TYPE(node->left());
        b = GET_TYPE(node->right());
        assert(a == INT || a == DOUBLE || a == BOOL);
        assert(b == INT || b == DOUBLE || b == BOOL);
        Type common = commonType(a, b);
        switch (node->kind()) {
            case tMUL:   // "*", 13)
                if (a == BOOL && b == BOOL) {
                    SET_TYPE(node, BOOL);
                    break;
                }
            case tADD:   // "+", 12)
            case tSUB:   // "-", 12)
            case tDIV:   // "/", 13)
                assert(common == BOOL || common == INT || common == DOUBLE);
                if (common == BOOL) {
                    common = INT;
                }
                SET_TYPE(node->left(), common);
                SET_TYPE(node->right(), common);
                SET_TYPE(node, common);
                break;
            case tMOD:   // "%", 13)
                SET_TYPE(node->left(), INT);
                SET_TYPE(node->right(), INT);
                SET_TYPE(node, INT);
                break;
            case tEQ:    // "==", 9)
            case tNEQ:   // "!=", 9)
            case tGT:    // ">", 10)
            case tGE:    // ">=", 10)
            case tLT:    // "<", 10)
            case tLE:    // "<=", 10)
                assert(common == BOOL || common == INT || common == DOUBLE);
                SET_TYPE(node->left(), common);
                SET_TYPE(node->right(), common);
                SET_TYPE(node, BOOL);
                break;
            case tOR:    // "||", 4)
            case tAND:   // "&&", 5)
                assert(common == BOOL || common == INT);
                SET_TYPE(node->left(), common);
                SET_TYPE(node->right(), common);
                SET_TYPE(node, BOOL);
                break;
            case tAOR:   // "|", 4)
            case tAAND:  // "&", 5)
            case tAXOR:  // "^", 5)
                if (a == BOOL && b == BOOL) {
                    SET_TYPE(node, BOOL);
                } else {
                    SET_TYPE(node->left(), INT);
                    SET_TYPE(node->right(), INT);
                    SET_TYPE(node, INT);
                }
                break;
            case tRANGE:
                SET_TYPE(node->left(), INT);
                SET_TYPE(node->right(), INT);
                SET_TYPE(node, NONE);
                break;
            default:
                assert(0);
        }
    }

    void visitUnaryOpNode(UnaryOpNode* node) {
        node->operand()->visit(this);
        Type argtp = GET_TYPE(node->operand());
        switch (node->kind()) {
            case tSUB:
                switch (argtp) {
                    case BOOL:
                    case INT:
                        SET_TYPE(node, INT);
                        break;
                    case DOUBLE:
                        SET_TYPE(node, DOUBLE);
                        break;
                    default:
                        assert(0);
                }
                break;
            case tNOT:
                if (argtp != BOOL) {
                    SET_TYPE(node->operand(), INT);
                }
                SET_TYPE(node, BOOL);
                break;
            default:
                break;
        }
    }

    void visitStringLiteralNode(StringLiteralNode* node) {
        SET_TYPE(node, STRING);
    }

    void visitDoubleLiteralNode(DoubleLiteralNode* node) {
        SET_TYPE(node, DOUBLE);
    }

    void visitIntLiteralNode(IntLiteralNode* node) {
        switch (node->literal()) {
            case 0:
            case 1:
                SET_TYPE(node, BOOL);
                break;
            default:
                SET_TYPE(node, INT);
                break;
        }
    }

    void visitLoadNode(LoadNode* node) {
        SET_TYPE(node, node->var()->type());
        accessVar(node->var());
    }

    void visitStoreNode(StoreNode* node) {
        SET_TYPE(node, NONE);
        node->value()->visit(this);
        accessVar(node->var());
    }

    void visitForNode(ForNode* node) {
        SET_TYPE(node, NONE);
        node->inExpr()->visit(this);
        node->body()->visit(this);
    }

    void visitWhileNode(WhileNode* node) {
        SET_TYPE(node, NONE);
        node->whileExpr()->visit(this);
        node->loopBlock()->visit(this);
        SET_TYPE(node->whileExpr(), BOOL);
    }

    void visitIfNode(IfNode* node) {
        SET_TYPE(node, NONE);
        node->ifExpr()->visit(this);
        node->thenBlock()->visit(this);
        if (node->elseBlock()) {
            node->elseBlock()->visit(this);
        }
        SET_TYPE(node->ifExpr(), BOOL);
    }

    void visitBlockNode(BlockNode* node) {
        SET_TYPE(node, NONE);
        currentScope = node->scope();

        Scope::VarIterator it(currentScope);
        while (it.hasNext()) {
            AstVar *next = it.next();
            registerVar(next);
        }

        Scope::FunctionIterator fit(currentScope);
        while (fit.hasNext()) {
            AstFunction *next = fit.next();
            enterFunction(next);
        }

        for (uint32_t i = 0; i < node->nodes(); i++) {
            node->nodeAt(i)->visit(this);
        }
        currentScope = currentScope->parent();
    }

    void visitFunctionNode(FunctionNode* node) {
        SET_TYPE(node, NONE);
        Scope *varScope = node->body()->scope()->parent();

        Scope::VarIterator it(varScope);
        while (it.hasNext()) {
            AstVar *next = it.next();
            registerVar(next);
        }

        node->body()->visit(this);
    }

    void visitReturnNode(ReturnNode* node) {
        SET_TYPE(node, NONE);
        AstNode *ex = node->returnExpr();
        if (ex) {
            ex->visit(this);
            SET_TYPE(ex, currentFunction->returnType());
        }
    }

    void visitCallNode(CallNode* node) {
        AstFunction *fn = currentScope->lookupFunction(node->name(), true);

        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            AstNode *p = node->parameterAt(i);
            p->visit(this);
            SET_TYPE(p, fn->parameterType(i));
        }
        SET_TYPE(node, (void *)(fn->returnType()));
    }

    void visitNativeCallNode(NativeCallNode* node) {
        SET_TYPE(node, NONE);
    }

    void visitPrintNode(PrintNode* node){
        SET_TYPE(node, NONE);
        for (uint32_t i = 0; i < node->operands(); ++i) {
            node->operandAt(i)->visit(this);
        }
    }

};

void assignTypes(AstFunction *top, Code *code) {
    ExprTyperVisitor visitor(code);
    visitor.enterFunction(top);
}

}
