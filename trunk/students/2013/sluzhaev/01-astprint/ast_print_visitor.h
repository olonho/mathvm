#ifndef AST_PRINT_VISITOR
#define AST_PRINT_VISITOR

#include "parser.h"
#include "ast.h"
#include "ast_print_visitor.h"

#include <sstream>

#define TAB_SIZE 4

using namespace mathvm;

struct AstPrintVisitor : public AstVisitor {
    AstPrintVisitor(std::ostream&);
    void printCode(const AstFunction*);
    
    virtual void visitBlockNode(BlockNode*);
    virtual void visitFunctionNode(FunctionNode*);
    virtual void visitNativeCallNode(NativeCallNode*);
    virtual void visitCallNode(CallNode*);
    virtual void visitUnaryOpNode(UnaryOpNode*);
    virtual void visitBinaryOpNode(BinaryOpNode*);
    virtual void visitIntLiteralNode(IntLiteralNode*);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode*);
    virtual void visitStringLiteralNode(StringLiteralNode*);
    virtual void visitLoadNode(LoadNode*);
    virtual void visitStoreNode(StoreNode*);
    virtual void visitForNode(ForNode*);
    virtual void visitWhileNode(WhileNode*);
    virtual void visitIfNode(IfNode*);
    virtual void visitReturnNode(ReturnNode*);
    virtual void visitPrintNode(PrintNode*);

private:
    void printIndent();
    
    unsigned int level;
    int last_precedence;
    std::ostream& out;
};

#endif
