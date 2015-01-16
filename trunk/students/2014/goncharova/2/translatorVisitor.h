#pragma once
#include "ast.h"
#include "common.h"
#include "mathvm.h"
#include "bytecodeScope.h"
#include "my_translator.h"

using namespace mathvm;

class TranslatorVisitor:public AstVisitor {
public:
    TranslatorVisitor(BytecodeScope * scope, BytecodeFunction * function);


#define VISITOR_FUNCTION(type, name)            \
            virtual void visit##type(type* node);

        FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
    Bytecode* bytecode();
    VarType tosType;
    void addBinOpBytecode(TokenKind tokenKind, VarType leftType, VarType rightType);
    void convertAndAddBinOp(TokenKind opKind, VarType leftType, VarType rightType);
    void addInstruction(Instruction instruction);
    void addId(idType id);
    void addDouble(double literal);
    void addInt(int64_t literal);
    void intToDouble();
    void convertBinOpArgs(VarType& leftType, VarType& rightType);
    void addSwap();
    void clearStack(AstNode* node);
    void castTo(VarType type);
    void castToDouble();
    void castToInt();
    void storeVariable(VarType variableType, idPair variableId);
    void loadVariable(idPair variableId);
    void branch(Instruction instruction, Label& label);
    BytecodeScope* currentScope;
    BytecodeFunction* currentFunction;
};