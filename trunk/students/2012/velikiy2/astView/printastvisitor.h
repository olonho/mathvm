#ifndef PRINTASTVISITOR_H
#define PRINTASTVISITOR_H

#include "mathvm.h"
#include "ast.h"
#include "visitors.h"

using namespace mathvm;

class PrintAstVisitor : public AstVisitor {

public:
    PrintAstVisitor();
    
    void visitTopFunction(const AstFunction*);
    
private:
    
    int myCurrentIndent;
    bool myBlockNoIndentBefore;
    bool myNoSemiAfter;
    
    virtual void visitBinaryOpNode(BinaryOpNode* node);
    virtual void visitUnaryOpNode(UnaryOpNode* node);
    virtual void visitStringLiteralNode(StringLiteralNode* node);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);
    virtual void visitIntLiteralNode(IntLiteralNode* node);    
    virtual void visitLoadNode(LoadNode* node);
    virtual void visitStoreNode(StoreNode* node);
    virtual void visitForNode(ForNode* node);
    virtual void visitWhileNode(WhileNode* node);
    virtual void visitIfNode(IfNode* node);
    virtual void visitBlockNode(BlockNode* node);
    virtual void visitFunctionNode(FunctionNode* node);
    virtual void visitReturnNode(ReturnNode* node);
    virtual void visitCallNode(CallNode* node);
    virtual void visitNativeCallNode(NativeCallNode* node);
    virtual void visitPrintNode(PrintNode* node);
    
    void printIndent();
    
    void printBlockInner(BlockNode* node);
    
    std::string getEscaped(const std::string& str);
    
};

#endif // PRINTASTVISITOR_H
