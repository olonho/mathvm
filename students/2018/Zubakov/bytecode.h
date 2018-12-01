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
        BytecodeFunction *_cur_function;
        Scope *cur_scope;
        uint16_t scope_id = 0;
        uint16_t var_number = 0;

        void cast(VarType left, VarType right);

        void generateComparison(BinaryOpNode *node);

        void generateComparisonSwitching(TokenKind kind);

        void generateBoolean(BinaryOpNode *node);

        void castToBoolean();

        void generateArithmetic(BinaryOpNode *node);

        void generateDoubleArithmetic(TokenKind tokenKind);

        void generateIntArithmetic(TokenKind tokenKind);

        void setLocalsInfo(BytecodeFunction *function);

        void setLocalsInfo();

        void storeVariable(const AstVar *var);

        void storeIntVariable(uint16_t vaId, uint16_t contextId);

        void storeDoubleVariable(uint16_t varId, uint16_t contextId);

        void storeStringVariable(uint16_t varId, uint16_t contextId);

        void loadVariable(const AstVar *var);

        void loadIntVariable(uint16_t varId, uint16_t contextId);

        void loadDoubleVariable(uint16_t varId, uint16_t contextId);

        void loadStringVariable(uint16_t varId, uint16_t contextId);

    public:
        void registerFunction(AstFunction* function);
        void visitAstFunction(AstFunction *astFunction);

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

        void visitUnaryOpNode(UnaryOpNode *node) override;

        void visitForNode(ForNode *node) override;

        void visitIfNode(IfNode *node) override;

        void visitWhileNode(WhileNode *node) override;

        void visitNativeCallNode(NativeCallNode *node) override;

        void visitReturnNode(ReturnNode *node) override;
    };

}
#endif //VIRTUAL_MACHINES_BYTECODE_H
