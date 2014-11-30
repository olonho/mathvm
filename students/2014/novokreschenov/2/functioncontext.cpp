#include "bytecodegenerator.h"
#include <stdint.h>

namespace mathvm {

FunctionTranslationContext::FunctionTranslationContext(uint16_t scopeId)
    : _scopeId(scopeId), _freeVarId(0)
{}

uint16_t FunctionTranslationContext::getFreeVarId()
{
    if (_freeVarId == UINT16_MAX) {
        throw TranslationException("too much variables");
    }
    return _freeVarId++;
}

void FunctionTranslationContext::registerScope(Scope* scope)
{
    registerScopeVars(scope);
}

void FunctionTranslationContext::registerScopeVars(Scope* scope) {
    Scope::VarIterator varIt(scope);
    while(varIt.hasNext()) {
        AstVar* var = varIt.next();
        uint16_t varId = getFreeVarId();
        addVarId(var->name(), varId);
    }
}

void FunctionTranslationContext::addVarId(const string &name, uint16_t id) {
    _varNameToIds[name].push_back(id);
}

uint16_t FunctionTranslationContext::addVar(const string &name) {
    uint16_t id = getFreeVarId();
    addVarId(name, id);

    return id;
}

void FunctionTranslationContext::unregisterScope(Scope *scope) {
    unregisterScopeVars(scope);
}

void FunctionTranslationContext::unregisterScopeVars(Scope *scope) {
    Scope::VarIterator varIt(scope);
    while(varIt.hasNext()) {
        AstVar* var = varIt.next();
        removeVarId(var->name());
    }
}

void FunctionTranslationContext::removeVarId(const string &name) {
    _varNameToIds[name].pop_back();

    if (_varNameToIds[name].size() == 0) {
        _varNameToIds.erase(name);
    }
}

void FunctionTranslationContext::registerSignature(const Signature &signature) {
    assert(signature.size() > 0);
    for (Signature::const_iterator it = signature.begin() + 1; it != signature.end(); ++it) {
        addVar(it->second);
    }
}

void FunctionTranslationContext::unregisterSignature(const Signature &signature) {
    assert(signature.size() > 0);
    for (Signature::const_iterator it = signature.begin() + 1; it != signature.end(); ++it) {
        removeVarId(it->second);
    }
}

bool FunctionTranslationContext::varNameExist(const string &varName) {
    return _varNameToIds.find(varName) != _varNameToIds.end();
}

uint16_t FunctionTranslationContext::getVarId(const string &varName) {
    return _varNameToIds[varName].back();
}

uint16_t FunctionTranslationContext::getScopeId() {
    return _scopeId;
}

ScopeVarId FunctionTranslationContext::getScopeVarId(const string &varName) {
    return ScopeVarId(getScopeId(), getVarId(varName));
}

}

