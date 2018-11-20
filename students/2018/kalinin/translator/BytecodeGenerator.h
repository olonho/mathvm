// Created by Владислав Калинин on 09/11/2018.
//

#ifndef MATHVM_BYTECODE_TRANSLATOR_VISITOR_H
#define MATHVM_BYTECODE_TRANSLATOR_VISITOR_H

#include "../../../../include/visitors.h"
#include "BytecodeInterpeter.h"
#include "Context.h"
#include <unordered_map>
#include <stdexcept>

using namespace std;

namespace mathvm {
    class Context;

    class BytecodeGenerator : public AstBaseVisitor {
        Context *ctx{};
        Bytecode *bytecode{};

    public:
        explicit BytecodeGenerator(Context *ctx, Bytecode *bytecode) : ctx(ctx), bytecode(bytecode) {};

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

        Bytecode *getBytecode();

    private :
        VarType getType(AstNode *node);

        void translateBooleanOperation(BinaryOpNode *node, TokenKind op);

        void translateCompareOperation(AstNode *left, AstNode *right, TokenKind op);

        void translateArithmeticOperation(BinaryOpNode *node, TokenKind op);

        void translateNegateNumber(UnaryOpNode *node);

        void translateInverseBoolean(UnaryOpNode *node);

        void translateLoadVariable(const AstVar *var);

        void translateStoreVariable(const AstVar *var);

        void translateCastTypes(VarType sourse, VarType target);

        void translateFunctionsBody(Scope *scope);

        bool isExpressionNode(AstNode *node);
    };

    class CompileError : std::exception {
        const char *msg;
        uint32_t position;

    public:
        CompileError(const char *msg, uint32_t position) : msg(msg), position(position) {}

        const char *getMsg() {
            return msg;
        }

        uint32_t getPosition() {
            return position;
        }
    };
}//mathvm

#endif //MATHVM_BYTECODE_TRANSLATOR_VISITOR_H