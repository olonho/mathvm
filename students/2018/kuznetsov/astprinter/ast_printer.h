#ifndef MATHVM_AST_PRINTER_H
#define MATHVM_AST_PRINTER_H

#include "ast.h"

namespace mathvm {

    class source_translator_impl: public Translator {
    public:
        source_translator_impl() = default;
        virtual Status* translate(const string& program, Code* *code);
    };

    class ast_printer_visitor : public AstVisitor {

    public:

        virtual void visitBinaryOpNode(BinaryOpNode* node) override;

        virtual void visitUnaryOpNode(UnaryOpNode* node) override;

        virtual void visitStringLiteralNode(StringLiteralNode* node) override;

        virtual void visitIntLiteralNode(IntLiteralNode* node) override;

        virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) override;

        virtual void visitLoadNode(LoadNode* node) override;

        virtual void visitStoreNode(StoreNode* node) override;

        virtual void visitBlockNode(BlockNode* node) override;

        virtual void visitNativeCallNode(NativeCallNode* node) override;

        virtual void visitForNode(ForNode* node) override;

        virtual void visitWhileNode(WhileNode* node) override;

        virtual void visitIfNode(IfNode* node) override;

        virtual void visitReturnNode(ReturnNode* node) override;

        virtual void visitFunctionNode(FunctionNode* node) override;

        virtual void visitCallNode(CallNode* node) override;

        virtual void visitPrintNode(PrintNode* node) override;

        void visit_topmost_function(AstFunction* top_function);
    };
}

#endif //MATHVM_AST_PRINTER_H
