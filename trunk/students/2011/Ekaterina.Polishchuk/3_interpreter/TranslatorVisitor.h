#ifndef FIRSTPASSVISITOR_H
#define FIRSTPASSVISITOR_H

#include "mathvm.h"
#include "ast.h"
#include "VariableScopeManager.h"

class TranslatorVisitor : public mathvm::AstVisitor {
private:
    bool myCurrentFunctionBlockNodeVisited;
public:
    virtual void visitUnaryOpNode(mathvm::UnaryOpNode* node);
    virtual void visitBinaryOpNode(mathvm::BinaryOpNode* node);
    virtual void visitStringLiteralNode(mathvm::StringLiteralNode* node);
    virtual void visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node);
    virtual void visitIntLiteralNode(mathvm::IntLiteralNode* node);
    virtual void visitLoadNode(mathvm::LoadNode* node);
    virtual void visitStoreNode(mathvm::StoreNode* node);
    virtual void visitForNode(mathvm::ForNode* node);
    virtual void visitWhileNode(mathvm::WhileNode* node);
    virtual void visitIfNode(mathvm::IfNode* node);
    virtual void visitBlockNode(mathvm::BlockNode* node);
    virtual void visitFunctionNode(mathvm::FunctionNode* node);
    virtual void visitPrintNode(mathvm::PrintNode* node);
    virtual void visitReturnNode(mathvm::ReturnNode* node);
    virtual void visitCallNode(mathvm::CallNode* node);

    void visit(mathvm::AstFunction * main);

    std::map<mathvm::AstNode const*, mathvm::VarType> myNodeTypes;
    std::deque<mathvm::FunctionNode const*> myFunctions;
    VariableScopeManager myScopeManager;
    mathvm::FunctionNode* myCurrentFunction;
};

#endif
