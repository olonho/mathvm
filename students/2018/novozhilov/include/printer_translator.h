#ifndef VIRTUAL_MACHINES_SOURCETRANSLATORIMPL_H
#define VIRTUAL_MACHINES_SOURCETRANSLATORIMPL_H

#include <mathvm.h>
#include <visitors.h>

namespace mathvm {
    class PrettyPrinterVisitor : public AstBaseVisitor {
    private:
        const string TAB = "    ";

        std::ostream &out;
        int indent;

        string getTab() const;
        void printIndent() const;

        void printVarType(VarType const& varType) const;
        void printTokenKind(TokenKind const& kind) const;
        void printTokenKindNoSpaces(TokenKind const& kind) const;
    public:
        explicit PrettyPrinterVisitor(std::ostream &out) : out(out), indent(-1) {}

        void visitTopNode(FunctionNode *node);

        void visitFunctionNode(FunctionNode *node) override;

        void visitFunctionNode(FunctionNode *node, bool isTop);

        void visitForNode(ForNode *node) override;

        void visitPrintNode(PrintNode *node) override;

        void visitLoadNode(LoadNode *node) override;

        void visitIfNode(IfNode *node) override;

        void visitBinaryOpNode(BinaryOpNode *node) override;

        void visitDoubleLiteralNode(DoubleLiteralNode *node) override;

        void visitStoreNode(StoreNode *node) override;

        void visitStringLiteralNode(StringLiteralNode *node) override;

        void visitWhileNode(WhileNode *node) override;

        void visitIntLiteralNode(IntLiteralNode *node) override;

        void visitUnaryOpNode(UnaryOpNode *node) override;

        void visitNativeCallNode(NativeCallNode *node) override;

        void visitBlockNode(BlockNode *node) override;

        void visitReturnNode(ReturnNode *node) override;

        void visitCallNode(CallNode *node) override;
    };

    class PrinterTranslator : public Translator {
    public:
        Status *translate(const string &program, Code **code) override;
    };

} // namespace mathvm

#endif //VIRTUAL_MACHINES_SOURCETRANSLATORIMPL_H
