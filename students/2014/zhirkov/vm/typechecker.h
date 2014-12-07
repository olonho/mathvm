#pragma once
#include <stack>
#include "../../../../vm/parser.h"
#include "ast_printer.h"
#include "ast_metadata.h"
#include "common.h"
#include "ast_utils.h"

namespace mathvm {

    inline VarType getType(AstNode* node) { return getData(node).type; }
    inline void setType(AstNode* node, VarType type) { getData(node).type = type; }

    class TypeChecker : AstVisitor {
    Scope* scope;
    public:
        TypeChecker() : scope(NULL) {}

        void visitAstFunction(AstFunction* fun);
        virtual void visitBinaryOpNode(BinaryOpNode *node);

        virtual void visitUnaryOpNode(UnaryOpNode *node);

        virtual void visitStringLiteralNode(StringLiteralNode *node);

        virtual void visitDoubleLiteralNode(DoubleLiteralNode *node);

        virtual void visitIntLiteralNode(IntLiteralNode *node);

        virtual void visitLoadNode(LoadNode *node);

        virtual void visitStoreNode(StoreNode *node);

        virtual void visitForNode(ForNode *node);

        virtual void visitWhileNode(WhileNode *node);

        virtual void visitIfNode(IfNode *node);

        virtual void visitBlockNode(BlockNode *node);

        virtual void visitFunctionNode(FunctionNode *node);

        virtual void visitReturnNode(ReturnNode *node);

        virtual void visitCallNode(CallNode *node);

        virtual void visitNativeCallNode(NativeCallNode *node);

        virtual void visitPrintNode(PrintNode *node);

        virtual ~TypeChecker() {}
    };


}