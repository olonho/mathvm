#pragma once

#include <iostream>
#include "../../../../include/ast.h"
#include "../../../../vm/parser.h"
#include "../../../../include/visitors.h"
namespace mathvm {

    class AstPrinterVisitor : public AstBaseVisitor {

    private:
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

        void indent();

        void _enter();

        void _leave();

        void functionDeclaration(Scope *scope);

        void variableDeclaration(Scope *scope);

        std::ostream& _out;
        size_t _indent;
        uint8_t _spacesForIndent;

    public:
        AstPrinterVisitor(
                std::ostream &out = std::cout,
                const uint8_t indentSpaces = 3) :
                _out(out),
                _indent(0),
                _spacesForIndent(indentSpaces) { }

        virtual void enterBlock(BlockNode *node);


    };



}