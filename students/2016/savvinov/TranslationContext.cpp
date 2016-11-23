//
// Created by dsavvinov on 11.11.16.
//

#include "TranslationContext.h"
#include "TranslationException.h"

namespace mathvm {

BytecodeFunction *TranslationContext::curFunction() {
    return functionsStack.top();
}

void TranslationContext::exitFunction() {
    functionsStack.pop();
}

void TranslationContext::enterFunction(BytecodeFunction *bytecodeFunction) {
    functionsStack.push(bytecodeFunction);
}

Scope *TranslationContext::curScope() {
    return scopesStack.top();
}

void TranslationContext::popScope() {
    scopesStack.pop();
}

void TranslationContext::pushScope(Scope * scope) {
    scopesStack.push(scope);
}

void TranslationContext::declareVariable(AstVar *var, Scope *pScope) {
    uint32_t varLocalId = curFunction()->localsNumber();
    curFunction()->setLocalsNumber((uint16_t) (varLocalId + 1));
    uint16_t curFunctionId = curFunction()->id();

    Info * info = new Info(varLocalId, curFunctionId, var->type());
    var->setInfo(info);

    Bytecode * bc = curFunction()->bytecode();
}

uint16_t TranslationContext::getCtxID(const AstVar *pVar) {
    return (uint16_t) static_cast<Info *>(pVar->info())->funcId;
}

Info * TranslationContext::getVarInfo(string const &name) {
    AstVar *var = curScope()->lookupVariable(name, /* useParent = */ true);
    return static_cast<Info*>(var->info());
}

Info::Info(uint32_t id, int16_t funcId, VarType type)
        : localId(id)
        , funcId(funcId)
        , type(type)
    {}
}