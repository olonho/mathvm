#pragma once

#include "ast.h"
#include "mathvm.h"
#include "visitors.h"

#include <map>
#include <queue>
#include <stack>


namespace mathvm {

struct BytecodeEmitterVisitor : AstBaseVisitor {
    BytecodeEmitterVisitor() = default;

    void setCode(Code *code);
    void emitFromTop(AstFunction* top);

#define VISITOR_FUNCTION(type, name)            \
void visit##type(type* node);

FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
    std::map<BytecodeFunction*, AstFunction*> byteToAstFunc;
    InterpreterCodeImpl* _code;
    std::queue<BytecodeFunction*> _functions;
    stack<VarType> _typeStack;
    BytecodeFunction* currentFunction = nullptr;
    
    std::map<Scope*, uint16_t> scopeToId;
    std::map<const AstVar*, uint16_t> varToId;

    bool needPop = false;

    VarType popType();

    uint16_t getVarId(const AstVar*);
    uint16_t getScopeId(Scope*);

    Bytecode* bytecode();

    void addFunction(AstFunction*);
    void enterFunction(BytecodeFunction*);
    void exitFunction();

    void addInstrsByKind(TokenKind);

    void castTOS(VarType, VarType);

    enum ConversionOp {
        GT_ZERO, NEQ_ZERO, LT_ZERO
    };
    void convertTOSIntToBoolean(ConversionOp);

    void doLogicalNot();
    void doComparison(TokenKind);

    void loadVar(const AstVar*);
};

}
