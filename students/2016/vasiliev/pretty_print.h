#ifndef MATHVM_PRETTY_PRINT_CPP_H
#define MATHVM_PRETTY_PRINT_CPP_H

#include <sstream>
#include "cstring"
#include "ast.h"
#include "mathvm.h"

class pretty_print: public mathvm::AstVisitor {

    std::stringstream ss;
    unsigned int indent;

public:
    pretty_print();

    virtual ~pretty_print() override;

    virtual void visitForNode(mathvm::ForNode *node) override;

    virtual void visitPrintNode(mathvm::PrintNode *node) override;

    virtual void visitLoadNode(mathvm::LoadNode *node) override;

    virtual void visitIfNode(mathvm::IfNode *node) override;

    virtual void visitCallNode(mathvm::CallNode *node) override;

    virtual void visitDoubleLiteralNode(mathvm::DoubleLiteralNode *node) override;

    virtual void visitStoreNode(mathvm::StoreNode *node) override;

    virtual void visitStringLiteralNode(mathvm::StringLiteralNode *node) override;

    virtual void visitWhileNode(mathvm::WhileNode *node) override;

    virtual void visitIntLiteralNode(mathvm::IntLiteralNode *node) override;

    virtual void visitBlockNode(mathvm::BlockNode *node) override;

    virtual void visitBinaryOpNode(mathvm::BinaryOpNode *node) override;

    virtual void visitUnaryOpNode(mathvm::UnaryOpNode *node) override;

    virtual void visitNativeCallNode(mathvm::NativeCallNode *node) override;

    virtual void visitReturnNode(mathvm::ReturnNode *node) override;

    virtual void visitFunctionNode(mathvm::FunctionNode *node) override;

    std::string get_text();
};

#endif
