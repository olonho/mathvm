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
    uint16_t localId;
    uint16_t funcId;
    VarType type;

    Info(uint32_t id, int16_t funcId, VarType type);
};
class TranslationContext {
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

    Info * getVarInfo(string const & name);

    void declareVariable(AstVar *pVar, Scope *pScope);

    uint16_t getCtxID(const AstVar *pVar);
};

} // mathvm namespace

#endif //MATHVM_TRANSLATIONCONTEXT_H
