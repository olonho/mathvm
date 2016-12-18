#pragma once

#include <map>
#include <utility>
#include <memory>
#include <stack>
#include <vector>

#include "ast.h"

#include "result_bytecode.h"
#include "variable_context.h"
#include "translation_exception.h"

using namespace mathvm;

class AstToBytecodeVisitor : public AstVisitor {
public:
    typedef std::stack<BytecodeFunction *> FunctionStack;
    typedef std::vector<VarType> VariableTypeStack;
    typedef std::vector<VariableContext> VariableContextStack;

    AstToBytecodeVisitor()
            : mCode(new ResultBytecode) {}

    ~AstToBytecodeVisitor() { delete mCode; }

    void visitTop(AstFunction *top);

    void visitAstFunction(AstFunction *astFunction);

    void visitBlockNode(BlockNode *node);

    void visitCallNode(CallNode *node);

    void visitFunctionNode(FunctionNode *node);

    void typeMismatch(AstNode *node);

    void getVariablesFromStackReverseOrder(FunctionNode *node);

    void visitReturnNode(ReturnNode *node);

    void visitStringLiteralNode(StringLiteralNode *node);

    void visitIntLiteralNode(IntLiteralNode *node);

    void visitDoubleLiteralNode(DoubleLiteralNode *node);

    void visitPrintNode(PrintNode *node);

    void visitLoadNode(LoadNode *node);

    void visitStoreNode(StoreNode *node);

    void visitUnaryOpNode(UnaryOpNode *node);

    void visitBinaryOpNode(BinaryOpNode *node);

    void visitIfNode(IfNode *node);

    void visitForNode(ForNode *node);

    void visitWhileNode(WhileNode *node);

    void iinc(ContextVariableId &id);

    Code *code();

private:
    Bytecode *currentBytecode() { return currentFunction()->bytecode(); }

    BytecodeFunction *currentFunction() { return mFunctionStack.top(); }

    void insn(Instruction instruction) { currentBytecode()->addInsn(instruction); }

    void int16(int16_t value) { currentBytecode()->addInt16(value); }

    void nop() {}

    void lor();

    void land();

    void eq();

    void neq();

    void notop();

    void gt();

    void iloadm1();

    void iload1();

    void lt();

    void ge();

    void le();

    void add();

    void sub();

    void mul();

    void div();

    void mod();

    void pop();

    void invalid();

    void icmp();

    void dcmp();

    void izero();

    void dzero();

    void imul();

    void dmul();

    void iaor();

    void iaand();

    void iaxor();

    void two2int();

    void two2double();

    void swap();

    ContextVariableId internalVariableId(int16_t internalId) {
        return std::make_pair(0, internalId);
    }


    VarType ttos() { return mVariableTypeStack.back(); }

    void pushTypeToStack(VarType type) { mVariableTypeStack.push_back(type); }

    void intTypeToStack() { pushTypeToStack(VT_INT); }

    void doubleTypeToStack() { pushTypeToStack(VT_DOUBLE); }

    void stringTypeToStack() { pushTypeToStack(VT_STRING); }

    void popTypeFromStack() { mVariableTypeStack.pop_back(); }

    void pop2TypesFromStack() {
        popTypeFromStack();
        popTypeFromStack();
    }

    void printOneOperand(AstNode *node);

    void varId(ContextVariableId &id);

    void storeVariable(VarType type, ContextVariableId &id);

    void storeInt(ContextVariableId &id);

    void storeDouble(ContextVariableId &id);

    void storeString(ContextVariableId &id);

    void loadVariable(VarType type, ContextVariableId &id);

    void loadInt(ContextVariableId &id);

    void loadDouble(ContextVariableId &id);

    void loadString(ContextVariableId &id);

    VariableContext &currentContext() { return mVariableContextStack.back(); }

    void newContext();

    void popContext();

    ContextVariableId findVariableInContexts(const std::string &name);

    uint16_t newVariable(const std::string name);

    bool typecastStackValueTo(VarType desiredType);

    void int2double();

    void double2int();

    bool typecastStackValuesToCommonType();

    void addVariables(BlockNode *node);

    void addFunctions(BlockNode *node);

    void visitFunctions(BlockNode *node);

    void pop2ndTop();

    void pop23thTop();

    void pop234thTop();

    string mangledFunctionName(const string &functionName) {
        return functionName;
    }

    bool isInLocalContext(ContextVariableId &id) {
        return currentContext().contextId() == id.first;
    }

    bool isTopFunction(FunctionNode *node) {
        return node->name() == "<top>";
    }

    void allowRange() {
        mRangeAllowed = true;
    }

    void disallowRange() {
        mRangeAllowed = false;
    }

    bool isRangeAllowed() { return mRangeAllowed; }

    bool isFunctionKnown(const std::string &name) {
        return mCode->functionByName(name) != nullptr;
    }

    bool mRangeAllowed = false;
    Code *mCode;
    FunctionStack mFunctionStack;
    VariableTypeStack mVariableTypeStack;
    VariableContextStack mVariableContextStack;
};

