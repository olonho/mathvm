//
// Created by aleks on 25.11.18.
//

#ifndef VIRTUAL_MACHINES_BYTECODE_H
#define VIRTUAL_MACHINES_BYTECODE_H

#include <visitors.h>

namespace mathvm {

    class BytecodeVisitor : public AstBaseVisitor {
    private:
        Code *const _code;
        Bytecode *bytecode;
        AstFunction *_cur_function;
        Scope *cur_scope;

        void cast(VarType left, VarType right);

        void generateDoubleArithmetic(TokenKind tokenKind);
        void generateIntArithmetic(TokenKind tokenKind);

    public:
        BytecodeVisitor(Code *code);

        void visitBlockNode(BlockNode *node) override;

        void visitFunctionNode(FunctionNode *node) override;

        void visitIntLiteralNode(IntLiteralNode *node) override;

        void visitPrintNode(PrintNode *node) override;

        void visitCallNode(CallNode *node) override;

        void visitDoubleLiteralNode(DoubleLiteralNode *node) override;

        void visitStringLiteralNode(StringLiteralNode *node) override;

        void visitBinaryOpNode(BinaryOpNode *node) override;

        void visitStoreNode(StoreNode *node) override;

        void visitLoadNode(LoadNode *node) override;
    };

}
#endif //VIRTUAL_MACHINES_BYTECODE_H
