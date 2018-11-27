//
// Created by Владислав Калинин on 15.10.2018.
//

#ifndef MATHVM_PRINT_VISITOR_H
#define MATHVM_PRINT_VISITOR_H

#include "../../../include/visitors.h"
#include <cstdarg>

namespace mathvm {
    class Print_visitor : public AstBaseVisitor {
        int indent = 0;

    public:
        void visitFunctionNode(FunctionNode *node) override;

        void visitBlockNode(BlockNode *node) override;

        void visitIfNode(IfNode *node) override;

        void visitWhileNode(WhileNode *node) override;

        void visitStoreNode(StoreNode *node) override;

        void visitLoadNode(LoadNode *node) override;

        void visitBinaryOpNode(BinaryOpNode *node) override;

        void visitUnaryOpNode(UnaryOpNode *node) override;

        void visitDoubleLiteralNode(DoubleLiteralNode *node) override;

        void visitIntLiteralNode(IntLiteralNode *node) override;

        void visitStringLiteralNode(StringLiteralNode *node) override;

        void visitForNode(ForNode *node) override;

        void visitReturnNode(ReturnNode *node) override;

        void visitPrintNode(PrintNode *node) override;

        void visitCallNode(CallNode *node) override;

        void visitNativeCallNode(NativeCallNode *node) override;

    private :
        void printFunctionInScope(BlockNode *node);

        void printVarInScope(BlockNode *node);

        void startVisit(FunctionNode *node);

        void visitFunction(FunctionNode *node);

        void visitBlocStatements(BlockNode *node);

        void printWithParens(AstNode *node);

        void printWithoutParens(AstNode *node);

        void printSingleArithmetic(AstNode *node);
    };


}//mathvm

#endif //MATHVM_PRINT_VISITOR_H
