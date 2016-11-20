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

void TranslationContext::declareVariable(AstVar *var, Scope *pScope) {
    uint32_t varLocalId = curFunction()->localsNumber();
    curFunction()->setLocalsNumber((uint16_t) (varLocalId + 1));
    uint16_t curFunctionId = curFunction()->id();

    Info * info = new Info(varLocalId, curFunctionId);
    var->setInfo(info);

    Bytecode * bc = curFunction()->bytecode();
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

uint16_t TranslationContext::getCtxID(const AstVar *pVar) {
    return (uint16_t) static_cast<Info *>(pVar->info())->funcId;
}

Info::Info(uint32_t id, int16_t funcId) : localId(id), funcId(funcId) {}
}