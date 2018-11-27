#ifndef MATHVM_SOURCE_TRANSLATOR_IMPL_H
#define MATHVM_SOURCE_TRANSLATOR_IMPL_H

#include <visitors.h>
#include <ast.h>
#include <parser.h>
#include <mathvm.h>
#include <iostream>
#include <string>


namespace mathvm {
    class SourceTranslatorImpl : public Translator {
    public:
        virtual Status *translate(const string &program, Code **code);
    };

    class MyVisitor : public AstBaseVisitor {

        static const int INDENT = 4;
        int indention = 0;

    public:

        void visitForNode(ForNode *node) override;

        void visitPrintNode(PrintNode *node) override;

        void visitLoadNode(LoadNode *node) override;

        void visitIfNode(IfNode *node) override;

        void visitBinaryOpNode(BinaryOpNode *node) override;

        void visitCallNode(CallNode *node) override;

        void visitDoubleLiteralNode(DoubleLiteralNode *node) override;

        void visitStoreNode(StoreNode *node) override;

        void visitStringLiteralNode(StringLiteralNode *node) override;

        void visitWhileNode(WhileNode *node) override;

        void visitIntLiteralNode(IntLiteralNode *node) override;

        void visitUnaryOpNode(UnaryOpNode *node) override;

        void visitNativeCallNode(NativeCallNode *node) override;

        void visitBlockNode(BlockNode *node) override;

        void visitReturnNode(ReturnNode *node) override;

        void visitFunctionNode(FunctionNode *node) override;

    private:

        void printIndention();

    };
}
#endif

