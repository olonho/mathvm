#pragma once

#include "visitors.h"

namespace mathvm {

class DetailedBinaryVisitor : public AstVisitor {
public :
    DetailedBinaryVisitor() {
        initTables();
    }
    virtual ~DetailedBinaryVisitor(){}

    virtual void visitBinaryOpNode(BinaryOpNode* node) {
        BinOpKind kind = binOpsKind[node->kind()];
        
        switch(kind) {
            case BIN_LOGIC:
                visitBooleanBinOpNode(node);
                break;
            case BIN_ARITHMETIC:
                visitArithmeticBinOpNode(node);
                break;
            case BIN_CMP:
                visitCmpBinOpNode(node);
                break;
            case BIN_RANGE:
                visitRangeBinOpNode(node);
                break;
        }
    }

    virtual void visitBooleanBinOpNode(BinaryOpNode* node) = 0;
    virtual void visitArithmeticBinOpNode(BinaryOpNode* node) = 0;
    virtual void visitCmpBinOpNode(BinaryOpNode* node) = 0;
    virtual void visitRangeBinOpNode(BinaryOpNode* node) = 0;

    virtual void visitUnaryOpNode(UnaryOpNode* node) = 0;
    virtual void visitStringLiteralNode(StringLiteralNode* node) = 0;
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) = 0;
    virtual void visitIntLiteralNode(IntLiteralNode* node) = 0;
    virtual void visitLoadNode(LoadNode* node) = 0;
    virtual void visitStoreNode(StoreNode* node) = 0;
    virtual void visitForNode(ForNode* node) = 0;
    virtual void visitWhileNode(WhileNode* node) = 0;
    virtual void visitIfNode(IfNode* node) = 0;
    virtual void visitBlockNode(BlockNode* node) = 0;
    virtual void visitFunctionNode(FunctionNode* node) = 0;
    virtual void visitReturnNode(ReturnNode* node) = 0;
    virtual void visitCallNode(CallNode* node) = 0;
    virtual void visitNativeCallNode(NativeCallNode* node) = 0;
    virtual void visitPrintNode(PrintNode* node) = 0;

protected:
    enum BinOpKind {
        BIN_LOGIC, BIN_CMP, BIN_ARITHMETIC, BIN_RANGE
    };

    BinOpKind binOpsKind[tTokenCount];

private:
    void initTables() {
        binOpsKind[tOR] = BIN_LOGIC;
        binOpsKind[tAND] = BIN_LOGIC;
        binOpsKind[tAOR] = BIN_ARITHMETIC;
        binOpsKind[tAAND] = BIN_ARITHMETIC;
        binOpsKind[tAXOR] = BIN_ARITHMETIC;
        binOpsKind[tEQ] = BIN_CMP;
        binOpsKind[tNEQ] = BIN_CMP;
        binOpsKind[tGT] = BIN_CMP;
        binOpsKind[tGE] = BIN_CMP;
        binOpsKind[tLT] = BIN_CMP;
        binOpsKind[tLE] = BIN_CMP;
        binOpsKind[tRANGE] = BIN_RANGE;
        binOpsKind[tADD] = BIN_ARITHMETIC;
        binOpsKind[tSUB] = BIN_ARITHMETIC;
        binOpsKind[tMUL] = BIN_ARITHMETIC;
        binOpsKind[tDIV] = BIN_ARITHMETIC;
        binOpsKind[tMOD] = BIN_ARITHMETIC;
    }
};
}
