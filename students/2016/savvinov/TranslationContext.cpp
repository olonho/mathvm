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

void TranslationContext::addFunction(BytecodeFunction *function, FunctionID funID) {
    functionsIDs[function->name()] = funID;
}

uint16_t TranslationContext::getVarID(AstVar const * var) {
    return *static_cast<uint16_t *>(var->info());
}

uint16_t TranslationContext::getVarID(const string &name) {
    AstVar *var = curScope()->lookupVariable(name);
    void * info = var->info();
    return *static_cast<uint16_t *>(info);
}

void TranslationContext::declareVariable(AstVar * var) {
    uint32_t * varLocalId = new uint32_t(curFunction()->localsNumber());
    curFunction()->setLocalsNumber(*varLocalId + 1);
    var->setInfo(varLocalId);
}

uint16_t TranslationContext::getFunID(const string &name) {
    auto iter = functionsIDs.find(name);
    if (iter == functionsIDs.end()) {
        throw TranslationException(
                (
                        string("Cant find ID of function ") +
                        name
                ).c_str()
        );
    }

    return iter->second;
}

}