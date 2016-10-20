//
// Created by dsavvinov on 16.10.16.
//

#ifndef MATHVM_PRINTERVISITOR_H
#define MATHVM_PRINTERVISITOR_H

#include "../../../include/visitors.h"

namespace mathvm {

class PrinterVisitor : public AstBaseVisitor {
private:
    uint32_t indent = 0;
    bool shouldIndent = true;

    string getIndent() {
        return shouldIndent ? string(indent, ' ') : "";
    }

    void Indent() {
        indent += 4;
    }
    void Outdent() {
        assert (indent >= 4);
        indent -= 4;
    }

    void printScope(Scope * s);
    void printLiteral(string const & s);
    bool isNeedingBrackets(AstNode *node, int thisPrecedence);
public:
    virtual void visitForNode(ForNode *node) override;
    virtual void visitPrintNode(PrintNode *node) override;
    virtual void visitLoadNode(LoadNode *node) override;
    virtual void visitIfNode(IfNode *node) override;
    virtual void visitBinaryOpNode(BinaryOpNode *node) override;
    virtual void visitCallNode(CallNode *node) override;
    virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) override;
    virtual void visitStoreNode(StoreNode *node) override;
    virtual void visitStringLiteralNode(StringLiteralNode *node) override;
    virtual void visitWhileNode(WhileNode *node) override;
    virtual void visitIntLiteralNode(IntLiteralNode *node) override;
    virtual void visitUnaryOpNode(UnaryOpNode *node) override;
    virtual void visitNativeCallNode(NativeCallNode *node) override;
    virtual void visitBlockNode(BlockNode *node) override;
    virtual void visitReturnNode(ReturnNode *node) override;

    virtual void visitFunctionNode(FunctionNode *node) override;
};

} // mathvm namespace

#endif //MATHVM_PRINTERVISITOR_H
