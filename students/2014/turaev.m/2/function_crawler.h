#ifndef FUNCTION_CRAWLER_H
#define FUNCTION_CRAWLER_H

#include "visitors.h"
#include "rich_function.h"
#include "interpreter_code_impl.h"
#include <stack>

using namespace mathvm;

class FunctionCrawler: public AstVisitor {
public:
    FunctionCrawler(Code *code): _code(code) { }

    virtual void visitBinaryOpNode(BinaryOpNode *node) {
        return;
    }

    virtual void visitUnaryOpNode(UnaryOpNode *node) {
        return;
    }

    virtual void visitStringLiteralNode(StringLiteralNode *node) {
        return;
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) {
        return;
    }

    virtual void visitIntLiteralNode(IntLiteralNode *node) {
        return;
    }

    virtual void visitLoadNode(LoadNode *node) {
        return;
    }

    virtual void visitStoreNode(StoreNode *node) {
        return;
    }

    virtual void visitForNode(ForNode *node) {
        node->body()->visit(this);
    }

    virtual void visitWhileNode(WhileNode *node) {
        node->loopBlock()->visit(this);
    }

    virtual void visitIfNode(IfNode *node) {
        node->thenBlock()->visit(this);
        if (node->elseBlock()) {
            node->elseBlock()->visit(this);
        }
    }

    virtual void visitBlockNode(BlockNode *node) {
        Scope::FunctionIterator it(node->scope());
        while (it.hasNext()) {
            AstFunction *currentFunction = it.next();
            _code->addFunction(new RichFunction(currentFunction));
            currentFunction->node()->visit(this);
        }

        node->visitChildren(this);
    }

    virtual void visitFunctionNode(FunctionNode *node) {
        node->visitChildren(this);
    }

    virtual void visitReturnNode(ReturnNode *node) {
        return;
    }

    virtual void visitCallNode(CallNode *node) {
        return;
    }

    virtual void visitNativeCallNode(NativeCallNode *node) {
        return;
    }

    virtual void visitPrintNode(PrintNode *node) {
        return;
    }

private:
    Code *_code;
};

#endif