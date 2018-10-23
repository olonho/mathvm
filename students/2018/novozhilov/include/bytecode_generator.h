//
// Created by jetbrains on 18.10.18.
//

#ifndef VIRTUAL_MACHINES_BYTECODE_INTERPRETER_H
#define VIRTUAL_MACHINES_BYTECODE_INTERPRETER_H

#include <visitors.h>
#include "context.h"

namespace mathvm {
    class BytecodeGenerator : public AstBaseVisitor {
    public:
        explicit BytecodeGenerator(Code* code);

        void generateCodeForTopFunction(AstFunction *function);

        void generateCodeForFunction(AstFunction *function);

        void visitForNode(ForNode *node) override;

        void visitWhileNode(WhileNode *node) override;

        void visitIfNode(IfNode *node) override;

        void visitPrintNode(PrintNode *node) override;

        void visitLoadNode(LoadNode *node) override;

        void visitStoreNode(StoreNode *node) override;

        void visitIntLiteralNode(IntLiteralNode *node) override;

        void visitDoubleLiteralNode(DoubleLiteralNode *node) override;

        void visitStringLiteralNode(StringLiteralNode *node) override;

        void visitUnaryOpNode(UnaryOpNode *node) override;

        void visitBinaryOpNode(BinaryOpNode *node) override;

        void visitCallNode(CallNode *node) override;

        void visitNativeCallNode(NativeCallNode *node) override;

        void visitBlockNode(BlockNode *node) override;

        void visitFunctionNode(FunctionNode *node) override;

        void visitReturnNode(ReturnNode *node) override;


    private:
        Code *_code;
        Context *_context;

        Bytecode *getBytecode();

        void storeValueToVar(AstVar const *var);

        void loadValueFromVar(AstVar const *var);

        VarType getTypeOnStackTop();

        void setTypeOnStackTop(VarType type);

        void castVarOnStackTop(VarType type);

        Context *findOwnerContextOfVar(string name);

        void processArithmeticOperation(BinaryOpNode *node);
        void processLogicOperation(BinaryOpNode *node);
        void processComparingOperation(BinaryOpNode *node);

        VarType processBinaryOperands(BinaryOpNode *node, bool castToInt = false);
    };


}

#endif //VIRTUAL_MACHINES_BYTECODE_INTERPRETER_H
