#ifndef BYTECODE_GENERATOR_H
#define BYTECODE_GENERATOR_H

#include "mathvm.h"
#include "ast.h"
#include "my_utils.h"
#include "interpreter_code_impl.h"

namespace mathvm {

class BytecodeGenerator : public AstVisitor {
private:
    InterpreterCodeImpl* _code;
    vector<uint16_t> _funIdsStack;
    vector<VarType> _typesStack;

public:
    BytecodeGenerator(InterpreterCodeImpl* code)
        : _code(code)
    {
        // just to avoid checks if stack is empty
        _typesStack.push_back(VT_INVALID);
    }

    void visitProgram(AstFunction* astFun);
    virtual void visitFunctionNode(FunctionNode* node);
    virtual void visitNativeCallNode(NativeCallNode *node);
    virtual void visitBlockNode(BlockNode* node);
    virtual void visitUnaryOpNode(UnaryOpNode* node);
    virtual void visitBinaryOpNode(BinaryOpNode* node);
    virtual void visitLoadNode(LoadNode* node);
    virtual void visitStoreNode(StoreNode* node);
    virtual void visitIntLiteralNode(IntLiteralNode* node);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);
    virtual void visitStringLiteralNode(StringLiteralNode* node);
    virtual void visitPrintNode(PrintNode* node);
    virtual void visitCallNode(CallNode* node);
    virtual void visitReturnNode(ReturnNode* node);
    virtual void visitIfNode(IfNode* node);
    virtual void visitWhileNode(WhileNode* node);
    virtual void visitForNode(ForNode* node);

private:
    typedef pair<bool, Instruction> MaybeInsn;

    void visitVarDecls(BlockNode* node);
    void visitFunDefs(BlockNode* node);
    void visitExprs(BlockNode* node);

    Bytecode* currentBcToFill() const {
        uint16_t topFunId = _funIdsStack.back();
        BytecodeFunction* funBc = (BytecodeFunction*) _code->functionById(topFunId);
        return funBc->bytecode();
    }

    shared_ptr<Context> currentCtx() {
        return _code->ctxById(_funIdsStack.back());
    }

    BytecodeFunction* currentBcFunction() {
        return (BytecodeFunction*) _code->functionById(_funIdsStack.back());
    }

    BytecodeFunction* createBytecodeFun(AstFunction* astFun);
    shared_ptr<Context> createContextWithArgs(FunctionNode* fNode, uint16_t transFunId);
    shared_ptr<Context> findVarInOuterCtx(string const& name);

    void genArgsStoreBc(Bytecode* bc, FunctionNode* node);
    bool genVarDeclBc(Bytecode* bc, AstVar* var, uint16_t varId);
    void genForNodeBc(Bytecode* bc, ForNode* node, uint16_t iterVarId);
    void genNotBc(Bytecode* bc, UnaryOpNode* node);

    void throwIfTypesIncompatible(size_t nodePos, VarType leftOpType, VarType rightOpType);
    void handleStrBinOps(BinaryOpNode* node, Bytecode* bc);
    void makeTypesSameIfNeeded(Bytecode* bc, VarType& leftOpType, VarType& rightOpType);
    bool handleBaseArithmOps(VarType leftOpType, BinaryOpNode* node, Bytecode* bc);
    bool handleIntArithmOps(VarType leftOpType, BinaryOpNode* node, Bytecode* bc);
    bool handleIntCompOps(VarType leftOpType, BinaryOpNode* node, Bytecode* bc);
    bool handleDoubleCompOps(VarType leftOpType, BinaryOpNode* node, Bytecode* bc);
    bool handleLogicOps(VarType leftOpType, BinaryOpNode* node, Bytecode* bc);
    bool handleRangeOp(VarType leftOpType, BinaryOpNode* node, Bytecode* bc);

    void castIfNeeded(VarType valueType, StoreNode* node, Bytecode* bc, VarType varType);
    void genStoreNodeBc(Bytecode* bc, string const& varName, VarType varType,
                        shared_ptr<Context> curCtx, StoreNode* node);
    void genAssignOpBc(StoreNode* node, Bytecode* bc, VarType varType);

    // general bc pieces generation utils
    void genBcInsnWithId(Bytecode* bc, Instruction insn, uint16_t id);
    void genBcInsnWithTwoIds(Bytecode* bc, Instruction insn,
                             uint16_t id1, uint16_t id2);
    void genTransferDataBc(Bytecode* bc, Instruction transfInsn,
                           vector<uint16_t> const& varsIds);
    void genInsnNTimes(Bytecode* bc, Instruction insn, size_t n);
    void genIncrementBc(Bytecode* bc, uint16_t varId,
                        Instruction bcLoadVar, Instruction bcLoad1,
                        Instruction bcAdd, Instruction bcStoreVar);
    void genBoolFromIficmp(Bytecode* bc, Instruction ificmp, size_t popsNum);
    void genConvertToBool(Bytecode* bc, Instruction bcLoad0, Instruction bcCmp);

    MaybeInsn typedInsn(VarType varType,
                                     Instruction intCaseInsn,
                                     Instruction doubleCaseInsn,
                                     Instruction stringCaseInsn);

    MaybeInsn typedInsnNumericsOnly(VarType varType,
                                                 Instruction intCaseInsn,
                                                 Instruction doubleCaseInsn);

    // error msg builders
    string multipleVarDeclMsg(string const& varName) const {
        return "multiple declaration of variable: " + varName;
    }
    string varNotDeclaredMsg(string const& varName) const {
        return "variable is not declared: " + varName;
    }
    string wrongTypeMsg() const {
        return "wrong expression type";
    }
    string wrongTypeMsg(VarType expectedType) const {
        return "wrong expression type, expected " + string(typeToName(expectedType));
    }
    string wrongParameterTypeMsg(string const& funName, size_t paramNumber) const {
        return "in function " + funName + ": invalid type of parameter " + to_string(paramNumber);
    }
    string wrongVarDeclMsg(string const& funName, string const& varName) {
        return "in function " + funName + ": variable " + varName + " is declared with invalid type";
    }
    string undefFunMsg(string const& funName) {
        return "undefined function " + funName;
    }
    string invalidUnaryOperatorMsg(TokenKind tok) {
        return "invalid unary operator " + string(tokenOp(tok));
    }
    string invalidStrOperationMsg() {
        return "invalid string operation, only == and != are valid";
    }
    string invalidTypeMsg() {
        return "invalid type encountered";
    }
    string binOpWithStrAndNonStrMsg() {
        return "binary operator is applied to string and non string operands";
    }
    string invalidBinOpMsg(TokenKind op) {
        return "invalid bunary operator " + string(tokenOp(op));
    }

    //visitProgram impl
    BytecodeFunction* initFun(AstFunction* astFun);
    void genFunBc(Bytecode *bc, FunctionNode *funNode);

    // visitCallNode impl
    void visitCallArguments(CallNode* node, TranslatedFunction* funToCall);
    void genCastBcOrThrow(Bytecode* bc, VarType expectedType, VarType actualType, string const& throwMsg);
    MaybeInsn getFstToSndCastBc(VarType fstType, VarType sndType);

    string invalidFunArgType(string const& funName, VarType expected, VarType actual) {
        string const expectedTypeName = string(typeToName(expected));
        string const actualTypeName = string(typeToName(actual));
        return "function '" + funName + "': invalid arg type in call, expected - " +
                expectedTypeName + ", actual - " + actualTypeName;
    }

    // visitReturnNode impl
    string invalidTypeToReturn(string const& funName, VarType expected, VarType actual) {
        string const expectedTypeName = string(typeToName(expected));
        string const actualTypeName = string(typeToName(actual));
        return "function " + funName + ": invalid type in 'return' expr, expected - " +
                expectedTypeName + ", actual - " + actualTypeName;
    }

    // visitForNode impl
    string wrongExprInForNode() {
        return "wrong expression in 'for', expected '..' (range operator)";
    }

    string invalidOpInForNode(TokenKind actualOp) {
        string const actualOpName = "'" + string(tokenOp(actualOp)) + "'";
        return "wrong binary operator in 'for', expected '..' (range operator), actual " + actualOpName;
    }

    string wrongIterVarTypeMsg(string const& iterVarName) {
        return "iteration variable " + iterVarName + " in 'for' has wrong type, only 'int' can be used";
    }
};

}

#endif // BYTECODE_GENERATOR_H
