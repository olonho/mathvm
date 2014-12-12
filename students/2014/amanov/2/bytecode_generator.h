#ifndef BYTECODE_GENERATOR_H
#define BYTECODE_GENERATOR_H

#include "generator_context_handler.h"
#include "ast.h"
#include "mathvm.h"

namespace mathvm {


class Code;

class BytecodeGenerator : public AstVisitor, public ErrorInfoHolder {
    Code *m_code;
    ContextHandler *m_currentContextHandler;
    bool m_callFromBlock;


    void addInsn(Instruction insn);
    void addIntOperandsBinaryInsn(AstNode *left, AstNode *right, Instruction insn);
    void error(const char* msg, ...);
    void convOperandIfNeed(VarType typeToConverse);
    void addCompareBinaryOp(AstNode *left, AstNode *right, TokenKind op);
    void assertTosType(VarType tosType);
    void addIntegerBinaryOp(AstNode *left, AstNode *right, TokenKind op);
    void addBinaryConvertibleInsn(AstNode *left, AstNode *right, Instruction insn);
    void addArithmeticBinaryOp(AstNode *left, AstNode *right, TokenKind op);
    void addLogicalBinaryOp(AstNode *left, AstNode *right, TokenKind op);
    void addLogicalNot(AstNode *operand);
    void addVarOperation(const AstVar *var, bool isLoad);
    void pushContext(AstFunction *function, BytecodeFunction *translatedFun);
    void popContext();
    void translateFunction(AstFunction *function);
    void addLoadVar(const AstVar *var) { addVarOperation(var, true); }
    void addStoreVar(const AstVar *var) { addVarOperation(var, false); }
    void declareFunctions(Scope *scope);
    void assertTypeIsNumeric(VarType type);
    Label label() { return Label(bytecode()); }


    bool isNumeric(VarType type) { return type == VT_DOUBLE || type == VT_INT; }
    bool isTosOperandType(VarType type) { return m_currentContextHandler->isTosOperandType(type); }
    void addBranch(Instruction insn, Label& label) { bytecode()->addBranch(insn, label); }
    void setTosOperandType(VarType type) { m_currentContextHandler->setTosOperandType(type); }
    void bind(Label& label) { bytecode()->bind(label); }
    Bytecode *bytecode() { return m_currentContextHandler->bytecode(); }
    VarType tosOperandType() { return m_currentContextHandler->tosOperandType(); }
    AstFunction *currentFunction() { return m_currentContextHandler->currentFunction(); }


public:
    BytecodeGenerator(Code *code)
        : m_code(code)
        , m_currentContextHandler(0)
        , m_callFromBlock(false)
    {}
    Status* generate(AstFunction *top);
    virtual ~BytecodeGenerator() {}
    virtual void visitBinaryOpNode(BinaryOpNode* node);
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

};

}

#endif // BYTECODE_GENERATOR_H
