#ifndef AST_TO_BYTECODE_VISITOR_H__
#define AST_TO_BYTECODE_VISITOR_H__

#include <map>
#include <utility>
#include <memory>
#include <stack>
#include <vector>

#include "../../../../../include/ast.h"

#include "bytecode.h"
#include "variable_context.h"
#include "translation_exception.h"

using namespace mathvm;

class ast_to_bytecode : public AstVisitor {
public:
    typedef std::stack<BytecodeFunction*> Stack;
    typedef std::vector <VarType> varTypeStack;
    typedef std::vector <variable_context> varStack;

    ast_to_bytecode();

    ~ast_to_bytecode();

    void visitTop(AstFunction *top);

    void visitAstFunction(AstFunction *astFunction);

    void visitBlockNode(BlockNode *node);

    void visitCallNode(CallNode *node);

    void visitFunctionNode(FunctionNode *node);

    void typeMismatch(AstNode *node);

    void getVariablesFro_stackReverseOrder(FunctionNode *node);

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

    void iinc(id_variable &id);

    void visitWhileNode(WhileNode *node);

    Code *code();

private:
    Bytecode *currentBytecode();

    BytecodeFunction *currentFunction();

    void insn(Instruction instruction);

    void int16(int16_t value);

    void nop();

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

    id_variable internalVariableId(int16_t internalId);


    VarType ttos();

    void pushTypeToStack(VarType type);

    void intTypeToStack();

    void doubleTypeToStack();

    void stringTypeToStack();

    void popTypeFro_stack();

    void pop2TypesFro_stack();

    void printOneOperand(AstNode *node);

    void varId(id_variable &id);

    void storeVariable(VarType type, id_variable &id);

    void storeInt(id_variable &id);

    void storeDouble(id_variable &id);

    void storeString(id_variable &id);

    void loadVariable(VarType type, id_variable &id);

    void loadInt(id_variable &id);

    void loadDouble(id_variable &id);

    void loadString(id_variable &id);

    variable_context &currentContext();

    void newContext();

    void popContext();

    id_variable findVariableInContexts(const std::string &name);

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

    string mangledFunctionName(const string &functionName);

    bool isInLocalContext(id_variable &id);

    bool isTopFunction(FunctionNode *node);

    void allowRange();

    void disallowRange();

    bool isRangeAllowed();

    bool isFunctionKnown(const std::string &name);

private:
    bool mRangeAllowed = false;
    Code *_code;
    Stack _stack;
    varTypeStack _varTypeStack;
    varStack _varStack;
};

#endif
