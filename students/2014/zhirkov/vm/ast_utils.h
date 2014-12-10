#pragma once

#include "../../../../include/ast.h"
#include "ast_metadata.h"

namespace mathvm {
    inline AstMetadata &getData(AstNode *node) {
        if (!node->info())
            node->setInfo(new AstMetadata());
        return *((AstMetadata *) (node->info()));
    }

    inline void clearData(AstNode *node) {
        delete &getData(node);
        node->setInfo(NULL);
    }


    class AstMetadataEraser : AstVisitor {
    public:
#define PURGE delete static_cast<AstMetadata*>(node->info());
        virtual void visitBinaryOpNode(BinaryOpNode *node) {
            node->visitChildren(this);
            PURGE
        }

        virtual void visitUnaryOpNode(UnaryOpNode *node) {
            node->visitChildren(this);
            PURGE
        }

        virtual void visitStringLiteralNode(StringLiteralNode *node) {
            node->visitChildren(this);
            PURGE
        }

        virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) {
            node->visitChildren(this);
            PURGE
        }

        virtual void visitIntLiteralNode(IntLiteralNode *node) {
            node->visitChildren(this);
            PURGE
        }

        virtual void visitLoadNode(LoadNode *node) {
            node->visitChildren(this);
            PURGE
        }

        virtual void visitStoreNode(StoreNode *node) {
            node->visitChildren(this);
            PURGE
        }

        virtual void visitForNode(ForNode *node) {
            node->visitChildren(this);
            PURGE
        }

        virtual void visitWhileNode(WhileNode *node) {
            node->visitChildren(this);
            PURGE
        }

        virtual void visitIfNode(IfNode *node) {
            node->visitChildren(this);
            PURGE
        }

        virtual void visitBlockNode(BlockNode *node) {
            node->visitChildren(this);
            PURGE
        }

        virtual void visitFunctionNode(FunctionNode *node) {
            node->visitChildren(this);
            PURGE
        }

        virtual void visitReturnNode(ReturnNode *node) {
            node->visitChildren(this);
            PURGE
        }

        virtual void visitCallNode(CallNode *node) {
            node->visitChildren(this);
            PURGE
        }

        virtual void visitNativeCallNode(NativeCallNode *node) {
            node->visitChildren(this);
            PURGE
        }

        virtual void visitPrintNode(PrintNode *node) {
            node->visitChildren(this);
            PURGE
        }

    };
}