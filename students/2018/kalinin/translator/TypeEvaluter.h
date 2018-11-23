//
// Created by Владислав Калинин on 20/11/2018.
//

#ifndef MATHVM_TYPEEVALUTER_H
#define MATHVM_TYPEEVALUTER_H

#include "../../../../include/visitors.h"
#include "BytecodeGenerator.h"

namespace mathvm {

    class TypeEvaluter : public AstBaseVisitor {
        Context *ctx{};
        VarType returnType = VT_VOID;

    public:
        explicit TypeEvaluter(Context *ctx) : ctx(ctx) {}

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

//        void visitNativeCallNode(NativeCallNode *node) override;

    private:
        void fillContext(Scope *scope);

        VarType checkCompareOperation(VarType left, VarType right);

        VarType checkEqualsOperation(VarType left, VarType right);

        VarType checkArithmeticOperation(VarType left, VarType right);

        VarType checkIntegerOperation(VarType left, VarType right);

        VarType checkRangeOperation(VarType left, VarType right);

        VarType checkFunctionCallParameter(VarType expected, VarType actual);

        void checkFunctionParameters(AstFunction *func);

        void setType(AstNode *node, VarType type);

        VarType getType(AstNode *node);

        bool containsFunction(string name);

        void visitFunctions(Scope *scope);
    };

}//mathvm

#endif //MATHVM_TYPEEVALUTER_H
