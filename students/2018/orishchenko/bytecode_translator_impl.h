#ifndef MATHVM_BYTECODE_TRANSLATOR_H
#define MATHVM_BYTECODE_TRANSLATOR_H

#include "interpreter_code.h"
#include <visitors.h>
#include <ast.h>
#include <parser.h>
#include <string>

namespace mathvm {

    class BytecodeVisitor : public AstBaseVisitor {
    private:

    public:
        BytecodeVisitor(AstFunction *top, InterpreterCode *code);

        ~BytecodeVisitor() override;

        void visitForNode(ForNode *node) override;

        void visitPrintNode(PrintNode *node) override;

        void visitLoadNode(LoadNode *node) override;

        void visitIfNode(IfNode *node) override;

        void visitCallNode(CallNode *node) override;

        void visitDoubleLiteralNode(DoubleLiteralNode *node) override;

        void visitStoreNode(StoreNode *node) override;

        void visitStringLiteralNode(StringLiteralNode *node) override;

        void visitWhileNode(WhileNode *node) override;

        void visitIntLiteralNode(IntLiteralNode *node) override;

        void visitBlockNode(BlockNode *node) override;

        void visitBinaryOpNode(BinaryOpNode *node) override;

        void visitUnaryOpNode(UnaryOpNode *node) override;

        void visitNativeCallNode(NativeCallNode *node) override;

        void visitReturnNode(ReturnNode *node) override;

        void visitFunctionNode(FunctionNode *node) override;

        InterpreterCode *code;
        stack<Bytecode *> bytecode;
        stack<VarType> typesStack;
        VisitorCtx *context;
        BytecodeFunction *cur_function;
        bool inside_function = false;

    private:

        VarType pop_type();

        void cast_top(VarType type);

        void comparingFunction(BinaryOpNode *node);

        void calcFunction(BinaryOpNode *node);

        void convert();

        void load(VarType type, const string &name);

        void save(VarType type, const string &name);

    };
}


#endif
