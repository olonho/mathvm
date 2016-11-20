//
// Created by dsavvinov on 11.11.16.
//

#ifndef MATHVM_TRANSLATIONCONTEXT_H
#define MATHVM_TRANSLATIONCONTEXT_H

#include <unordered_map>
#include <stack>
#include "typedefs.h"
#include "../../../include/mathvm.h"
#include "../../../include/ast.h"
#include "BytecodeCode.h"

namespace mathvm {

class Info {
public:
    uint32_t localId;
    int16_t funcId;

    Info(uint32_t id, int16_t funcId);
};
class TranslationContext {
    unordered_map<string, FunctionID> functionsIDs {};
    unordered_map<string, uint16_t> stringIDs {};
    stack<BytecodeFunction *> functionsStack {};
    stack<Scope *> scopesStack {};
public:
    VarType tosType;
    BytecodeFunction * curFunction();
    void exitFunction();
    void enterFunction(BytecodeFunction * bytecodeFunction);

    Scope * curScope();
    void pushScope(Scope * scope);
    void popScope();

    void addFunction(BytecodeFunction * function, FunctionID funID);

    uint16_t getVarID(const string &basic_string);
    uint16_t getVarID(AstVar const * var);
    uint16_t getFunID(const string & name);

    void declareVariable(AstVar *pVar, Scope *pScope);

    uint16_t getCtxID(const AstVar *pVar);
};

} // mathvm namespace

#endif //MATHVM_TRANSLATIONCONTEXT_H
