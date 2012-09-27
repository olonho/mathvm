#ifndef ____PrintVisitor__
#define ____PrintVisitor__

#include "visitors.h"
#include "ast.h"

using namespace mathvm;

class PrintVisitor : public AstVisitor {
    int indent;
public:
    PrintVisitor():indent(0) {}
    virtual ~PrintVisitor() {}
    
    virtual void visitBinaryOpNode(BinaryOpNode *node);
    virtual void visitUnaryOpNode(UnaryOpNode *node);
    virtual void visitStringLiteralNode(StringLiteralNode *node);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode *node);
    virtual void visitIntLiteralNode(IntLiteralNode *node);
    virtual void visitLoadNode(LoadNode *node);
    virtual void visitStoreNode(StoreNode *node);
    virtual void visitForNode(ForNode *node);
    virtual void visitWhileNode(WhileNode *node);
    virtual void visitIfNode(IfNode *node);
    virtual void visitBlockNode(BlockNode *node);
    virtual void visitFunctionNode(FunctionNode *node);
    virtual void visitReturnNode(ReturnNode *node);
    virtual void visitCallNode(CallNode *node);
    virtual void visitNativeCallNode(NativeCallNode *node);
    virtual void visitPrintNode(PrintNode *node);
};

#endif
