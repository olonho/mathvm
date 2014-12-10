#ifndef BYTECODE_GENERATOR_H
#define BYTECODE_GENERATOR_H

#include <vector>
#include <stack>

#include "ast.h"
#include "mathvm.h"
#include "interpreter_code_impl.h"

using std::vector;
using std::stack;
using namespace mathvm;

//struct Context {
//    uint16_t id;
//    map<string, uint16_t> vars;
//};

//enum VarCoordsType { CT_NONE, CT_LOCAL, CT_WITH_CTX };

//struct VarCoords {
//    VarCoordsType type;
//    uint16_t varId;
//    uint16_t ctxId;
//};

//class GeneratorState {
//private:
//    InterpreterCodeImpl _code;

//    stack<uint16_t> _functions;

//    vector<Context> _contexts;
//    uint16_t _currentVarId;

//public:
//    GeneratorState(InterpreterCodeImpl* code)
//        : _code(code)
//        , _currentVarId(0)
//    {}

//    BytecodeFunction* currentBcFunction() {
//        return (BytecodeFunction*) _code->functionById(_functions.top());
//    }
//    Bytecode* currentBcToFill() {
//        return currentBcFunction()->bytecode();
//    }
//    Context& currentContext() { return _contexts.back(); }

//    uint16_t currentCtxAddVar(string const& name, size_t nodePos) {
//        if(_currentVarId == UINT16_MAX) {
//            throw ExceptionWithPos(tooMuchVarsMsg(), nodePos);
//        }
//        Context& current = _contexts.back();
//        auto res = current.vars.insert(make_pair(name, _currentVarId));
//        if(!res.second) {
//            throw ExceptionWithPos(varDoubleDeclMsg(name), nodePos);
//        }
//        return _currentVarId ++;
//    }

//    VarCoords findVar(string const& name) {
//        VarCoords coords;
//        coords.type = CT_NONE;
//        for(size_t i = _contexts.size(); i > 0; --i) {
//            Context& current = _contexts[i - 1];
//            auto res = current.vars.find(name);
//            if(res != current.vars.end()) {
//                coords.varId = res->second;
//                if(i == _contexts.size()) {
//                    coords.type = CT_LOCAL;
//                } else {
//                    coords.type = CT_WITH_CTX;
//                    coords.ctxId = current.id;
//                }
//            }
//        }
//        return coords;
//    }

//    uint16_t createBcFun(AstFunction* astFun) {
//        BytecodeFunction* bcFun = new BytecodeFunction(astFun);
//        return _code->addFunction(bcFun);
//    }

//private:
//    string tooMuchVarsMsg() {
//        return "too much variables created, this VM doesn't support more than 65535 variables per context";
//    }
//    string varDoubleDeclMsg(string const& name) {
//        return "variable " + name + " is already declared";
//    }
//};

class BytecodeGenerator : public AstVisitor {
private:
    InterpreterCodeImpl* _code;

    struct Context {
        uint16_t id;
        map<string, uint16_t> vars;
    };
    vector<Context> _contexts;
    uint16_t _currentVarId;

    stack<uint16_t> _functions;

    ExecStatus _status;

public:
    BytecodeGenerator(InterpreterCodeImpl* code)
        : _code(code)
        , _currentVarId(0)
    {}

    void gen(AstFunction* top);
    ExecStatus const & status() { return _status; }

    virtual void visitFunctionNode(FunctionNode* node);
    virtual void visitBlockNode(BlockNode* node);
    virtual void visitReturnNode(ReturnNode* node);
    virtual void visitNativeCallNode(NativeCallNode* node);
    virtual void visitCallNode(CallNode* node);
    virtual void visitPrintNode(PrintNode* node);
    virtual void visitUnaryOpNode(UnaryOpNode* node);
    virtual void visitBinaryOpNode(BinaryOpNode* node);
    virtual void visitIntLiteralNode(IntLiteralNode* node);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);
    virtual void visitStringLiteralNode(StringLiteralNode* node);
    virtual void visitLoadNode(LoadNode* node);
    virtual void visitStoreNode(StoreNode* node);
    virtual void visitForNode(ForNode* node);
    virtual void visitWhileNode(WhileNode* node);
    virtual void visitIfNode(IfNode* node);

private:
    BytecodeFunction* currentBcFunction() {
        return (BytecodeFunction*) _code->functionById(_functions.top());
    }
    Bytecode* currentBcToFill() {
        return currentBcFunction()->bytecode();
    }

    // working with contexts
    Context& currentContext() { return _contexts.back(); }
    uint16_t currentCtxAddVar(string const& name, size_t nodePos);
    string tooMuchVarsMsg() {
        return "too much variables created, this VM doesn't support more than 65535 variables per context";
    }
    string varDoubleDeclMsg(string const& name) {
        return "variable " + name + " is already declared";
    }

    enum VarCoordsType { CT_NONE, CT_LOCAL, CT_WITH_CTX };
    struct VarCoords {
        VarCoordsType type;
        uint16_t varId;
        uint16_t ctxId;
    };
    VarCoords findVar(string const& name);

    // visitBlockNode impl
    void visitVarDecls(BlockNode* node);
    void visitFunDefs(BlockNode* node);
    uint16_t createBcFun(AstFunction* astFun);
    void pushNewContextWithArgs(uint16_t id, FunctionNode* node);

    // visitPrintNode impl
    Instruction typedInsn(VarType type, Instruction intCase, Instruction doubleCase, Instruction strCase) {
        if     (type == VT_INT)    { return intCase; }
        else if(type == VT_DOUBLE) { return doubleCase; }
        return strCase;
    }

    // visitUnaryOpNode impl
    string invalidUnaryOperatorMsg(TokenKind unOp) {
        return "invalid unary operator " + string(tokenOp(unOp));
    }
    void genNotBc(Bytecode* bc);
    void genBoolFromIficmp(Bytecode* bc, Instruction ificmp, size_t popsNum);
    void genRepeatInsn(Bytecode* bc, Instruction insn, size_t n);

    // visitBinaryOpNode impl
    string invalidBinOpMsg(TokenKind op) {
        return "invalid bunary operator " + string(tokenOp(op));
    }
    void genBaseArithmOpBc(Bytecode* bc, VarType rightOpType, VarType leftOpType,
                           Instruction intOp, Instruction doubleOp);
    void genOperandsCastBc(Bytecode* bc, VarType leftOpType);
    void getCompOpBc(Bytecode* bc, VarType rightOpType, VarType leftOpType,
                     Instruction intIfcmp, Instruction doubleIfcmp);
    void genConvertIntToBoolBc(Bytecode* bc);

    // visitStringLiteralNode impl
    void genBcInsnWithId(Bytecode* bc, Instruction insn, uint16_t id);

    // visitLoadNode impl
    void genLoadBc(VarType type, VarCoords& coords) {
        genTransferBc(type, coords,
                      BC_LOADIVAR, BC_LOADDVAR, BC_LOADSVAR,
                      BC_LOADCTXIVAR, BC_LOADCTXDVAR, BC_LOADCTXSVAR);
    }
    void genTransferBc(VarType type, VarCoords& coords,
                      Instruction transIVar, Instruction transDVar, Instruction transSVar,
                      Instruction transCtxIVar, Instruction transCtxDVar, Instruction transCtxSVar);

    void genBcInsnWithTwoIds(Bytecode* bc, Instruction insn,
                             uint16_t id1, uint16_t id2);

    // visitStoreNode impl
    void genStoreBc(VarType type, VarCoords& coords) {
        genTransferBc(type, coords,
                      BC_STOREIVAR, BC_STOREDVAR, BC_STORESVAR,
                      BC_STORECTXIVAR, BC_STORECTXDVAR, BC_STORECTXSVAR);
    }

    // visitForNode impl
    void throwIfNotRangeOp(ForNode* node);
    string wrongInExprMsg() {
        return "wrong 'in' expr in for, range operator '..' expected";
    }
    void genIntIncrBc(VarCoords& intVarCoords);
};

#endif // BYTECODE_GENERATOR_H
