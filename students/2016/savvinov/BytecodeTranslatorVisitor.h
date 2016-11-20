//
// Created by dsavvinov on 11.11.16.
//

#ifndef MATHVM_BYTECODETRANSLATORVISITOR_H
#define MATHVM_BYTECODETRANSLATORVISITOR_H

#include "../../../include/mathvm.h"
#include "../../../include/visitors.h"
#include "BytecodeCode.h"
#include "TranslationContext.h"

namespace  mathvm {
class BytecodeTranslatorVisitor : public AstBaseVisitor {
private:

    void verifyRangeExpr(AstNode *pNode);

    template <class T>
    void assignConstant(uint16_t id, VarType anInt, T value);
    void increment(uint16_t i);

    void storeParams(AstFunction *pFunction);
    template <class T>
    void loadConstant(VarType type, T value);
    void storeIntoVariable(VarType type, uint16_t id);
    void loadFromLocal(VarType type, uint16_t id);
    void visitScope(Scope * scope);
    VarType visitExpressionWithResult(AstNode *pNode);

public:
    BytecodeTranslatorVisitor();

    BytecodeCode * code;
    TranslationContext ctx;

    BytecodeCode *getCode() const;

    virtual void visitWhileNode(WhileNode *node) override;
    virtual void visitForNode(ForNode *node) override;
    virtual void visitPrintNode(PrintNode *node) override;
    virtual void visitLoadNode(LoadNode *node) override;
    virtual void visitStoreNode(StoreNode *node) override;
    virtual void visitIfNode(IfNode *node) override;
    virtual void visitBinaryOpNode(BinaryOpNode *node) override;
    virtual void visitCallNode(CallNode *node) override;
    virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) override;
    virtual void visitStringLiteralNode(StringLiteralNode *node) override;
    virtual void visitIntLiteralNode(IntLiteralNode *node) override;
    virtual void visitUnaryOpNode(UnaryOpNode *node) override;
    virtual void visitNativeCallNode(NativeCallNode *node) override;
    virtual void visitBlockNode(BlockNode *node) override;
    virtual void visitReturnNode(ReturnNode *node) override;
    virtual void visitFunctionNode(FunctionNode *node) override;

    void incrementIntoVariable(VarType type, uint16_t id);

    void decrementIntoVariable(VarType type, uint16_t id);

    void compare(Instruction insn, VarType type);

    bool pushesOnStack(AstNode *pNode);

    void castType(VarType type, VarType returnType);

    void loadFromContext(VarType type, uint16_t id, uint16_t ctxId);
};

}
#endif //MATHVM_BYTECODETRANSLATORVISITOR_H
