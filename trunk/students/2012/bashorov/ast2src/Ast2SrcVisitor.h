#ifndef AST2SRC_VISITOR_H
#define AST2SRC_VISITOR_H

#include <iostream>

#include "visitors.h"

namespace mathvm {

class Ast2SrcVisitor : public AstVisitor {
public:
    Ast2SrcVisitor(std::ostream& out = std::cout);

private:
    virtual void visitBinaryOpNode(BinaryOpNode* node);
    virtual void visitUnaryOpNode(UnaryOpNode* node);

    virtual void visitStringLiteralNode(StringLiteralNode* node);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);
    virtual void visitIntLiteralNode(IntLiteralNode* node);

    virtual void visitLoadNode(LoadNode* node);
    virtual void visitStoreNode(StoreNode* node);
    virtual void visitBlockNode(BlockNode* node);

    virtual void visitForNode(ForNode* node);
    virtual void visitWhileNode(WhileNode* node);
    virtual void visitIfNode(IfNode* node);

    virtual void visitFunctionNode(FunctionNode* node);
    virtual void visitReturnNode(ReturnNode* node);
    virtual void visitCallNode(CallNode* node);
    virtual void visitNativeCallNode(NativeCallNode* node);
    virtual void visitPrintNode(PrintNode* node);

    void initScope(Scope* scope);
    void outSignatureParams(const Signature& signature);

    std::ostream& _out;
};

}
#endif
