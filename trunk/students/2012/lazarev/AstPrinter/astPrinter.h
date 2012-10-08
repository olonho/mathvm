#ifndef _MATHVM_AST_PRINTER
#define _MATHVM_AST_PRINTER

#include "ast.h"

using namespace mathvm;

class AstPrinter: public AstVisitor {

    bool blockEnded;

    std::string opToString(TokenKind tokenKind);
public:
    AstPrinter() : blockEnded(false) {}
    virtual ~AstPrinter() {}
    
    
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
};

#endif
