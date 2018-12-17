#pragma once

#include <visitors.h>
#include "context.hpp"

namespace mathvm {

    class BytecodeTranslatorVisitor : public AstBaseVisitor {
    public:

        explicit BytecodeTranslatorVisitor(Code* code);
        void translateProgram(AstFunction* function);
        void visitForNode(ForNode* node) override;
        void visitPrintNode(PrintNode* node) override;
        void visitLoadNode(LoadNode* node) override;
        void visitIfNode(IfNode* node) override;
        void visitBinaryOpNode(BinaryOpNode* node) override;
        void visitCallNode(CallNode* node) override;
        void visitDoubleLiteralNode(DoubleLiteralNode* node) override;
        void visitStoreNode(StoreNode* node) override;
        void visitStringLiteralNode(StringLiteralNode* node) override;
        void visitWhileNode(WhileNode* node) override;
        void visitIntLiteralNode(IntLiteralNode* node) override;
        void visitUnaryOpNode(UnaryOpNode* node) override;
        void visitNativeCallNode(NativeCallNode* node) override;
        void visitBlockNode(BlockNode* node) override;
        void visitReturnNode(ReturnNode* node) override;
        void visitFunctionNode(FunctionNode* node) override;

    private:
        Code* const code_;
        std::vector<CallContext> contexts_;

        void translateFunction(AstFunction* pFunction);
        BytecodeFunction* createFunction(AstFunction* function);

        Bytecode* currentBytecode();
        CallContext& context();
        CallContext const& locateContextWithVariable(string const& name) const;

        void binaryMathOperation(BinaryOpNode* node);

        void enterContext(BytecodeFunction* function);
        void leaveContext();

        void loadVar(AstVar const& var);
        void storeVar(AstVar const& var);

        void binaryBooleanLogicOperation(BinaryOpNode* pNode);
        void binaryRelationOperation(const BinaryOpNode* pNode);
        void binaryMathLogicOperation(BinaryOpNode const* pNode);
        void negateAsBool();
        void castFromTo(VarType from, VarType to);
        void castToBool();

        bool isUnusedExpression(AstNode* child) const;
    };
}