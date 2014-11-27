#pragma once

#include "detailed_binary_visitor.h"

namespace mathvm {

struct NodeData {
    VarType type;

    NodeData(VarType type) : type(type) {}

    static NodeData* getTyped(VarType type = VT_INVALID) {
        switch(type) {
            case VT_INT:
                return &node_int;
            case VT_DOUBLE:
                return &node_double;
            case VT_STRING:
                return &node_string;
            case VT_VOID:
                return &node_void;
            case VT_INVALID:
            default:
                return &node_invalid;
        }
    }

private:
    static NodeData node_int, node_double, node_string, node_void, node_invalid;
};


class ValidatingVisitor : public DetailedBinaryVisitor {
public:
    ValidatingVisitor() {}
    
    virtual ~ValidatingVisitor() {}

    Status* checkProgram(AstFunction* top);

    bool isBinaryOk(VarType left, VarType right, TokenKind op);


    virtual void visitBooleanBinOpNode(BinaryOpNode* node);
    virtual void visitArithmeticBinOpNode(BinaryOpNode* node);
    virtual void visitCmpBinOpNode(BinaryOpNode* node);
    virtual void visitRangeBinOpNode(BinaryOpNode* node);
    virtual void visitBinaryOpNode(BinaryOpNode* node);
    void checkBinOpNode(BinaryOpNode* node, bool intOnly);

    bool isUnaryOk(VarType type, TokenKind token);
    virtual void visitUnaryOpNode(UnaryOpNode* node);

    virtual void visitStringLiteralNode(StringLiteralNode* node);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);
    virtual void visitIntLiteralNode(IntLiteralNode* node);
    virtual void visitLoadNode(LoadNode* node);
    virtual void visitStoreNode(StoreNode* node);
    virtual void visitForNode(ForNode* node);
    virtual void visitWhileNode(WhileNode* node);
    virtual void visitIfNode(IfNode* node);
    virtual void visitBlockNode(BlockNode* node);
    virtual void visitFunctionNode(FunctionNode* node);
    virtual void visitReturnNode(ReturnNode* node);
    virtual void visitCallNode(CallNode* node);
    virtual void visitNativeCallNode(NativeCallNode* node);
    virtual void visitPrintNode(PrintNode* node);

    void checkAstFunction(AstFunction* function);
    void checkAstVar(AstVar* var);
    void checkScope(Scope* scope);
    
private:
    void expectType(AstNode* node, VarType type);
    
    void fail(string const & msg, uint32_t position);
    void typeFail(VarType expected, VarType found, uint32_t position);
    void binOpFail(BinaryOpNode* node);

    bool isLvalueType(VarType type);

    Status* status;
    AstFunction* currentFunction;
    Scope* currentScope;
};
}
