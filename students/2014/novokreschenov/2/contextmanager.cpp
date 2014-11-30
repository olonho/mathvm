#include "contextmanager.h"


namespace mathvm {

ContextVar::ContextVar()
    :_type(VT_INVALID) {
}

ContextVar::ContextVar(double d)
    : _type(VT_DOUBLE) {
    _value.d = d;
}

ContextVar::ContextVar(int64_t i)
    : _type(VT_INT) {
    _value.i = i;
}

ContextVar::ContextVar(uint16_t id)
    : _type(VT_STRING) {
    _value.id = id;
}

VarType ContextVar::type() {
    return _type;
}

double ContextVar::doubleValue() {
    assert(_type == VT_DOUBLE);
    return _value.d;
}

int64_t ContextVar::intValue() {
    assert(_type == VT_INT);
    return _value.i;
}

uint16_t ContextVar::stringIdValue() {
    assert(_type == VT_STRING);
    return _value.id;
}

void ContextVar::setDouble(double d) {
    _type = VT_DOUBLE;
    _value.d = d;
}

void ContextVar::setInt(int64_t i) {
    _type = VT_INT;
    _value.i = i;
}

void ContextVar::setStringId(uint16_t id) {
    _type = VT_STRING;
    _value.id = id;
}

void InterpreterStack::pushElement(ContextVar element) {
    _elements.push_back(element);
}

void InterpreterStack::pushDouble(double d) {
    _elements.push_back(ContextVar(d));
}

void InterpreterStack::pushInt(int64_t i) {
    _elements.push_back(ContextVar(i));
}

void InterpreterStack::pushStringId(uint16_t id) {
    _elements.push_back(ContextVar(id));
}

ContextVar InterpreterStack::getElement() {
    assert(_elements.size() > 0);
    return _elements.back();
}

double InterpreterStack::getDouble() {
    assert(_elements.size() > 0);
    return _elements.back().doubleValue();
}

int64_t InterpreterStack::getInt() {
    assert(_elements.size() > 0);
    return _elements.back().intValue();
}

uint16_t InterpreterStack::getStringId() {
    assert(_elements.size() > 0);
    return _elements.back().stringIdValue();
}

ContextVar InterpreterStack::popElement() {
    assert(_elements.size() > 0);
    ContextVar element = _elements.back();
    _elements.pop_back();

    return element;

}

double InterpreterStack::popDouble() {
    double d = getDouble();
    popElement();

    return d;
}

int64_t InterpreterStack::popInt() {
    int64_t i = getInt();
    popElement();

    return i;
}

uint16_t InterpreterStack::popStringId() {
    uint16_t id = getStringId();
    popElement();

    return id;
}

void InterpreterStack::swapTwoUpperElements() {
    ContextVar upper = popElement();
    ContextVar lower = popElement();
    pushElement(upper);
    pushElement(lower);
}

#include <cstring>
void ContextManager::checkContext(uint16_t contextId) {
    ContextMap::iterator contextIt = _contextById.find(contextId);
    if (contextIt == _contextById.end()) {
        throw InterpretationException("Context doesn't exist");
    }
}

void ContextManager::checkCtxVar(uint16_t contextId, uint16_t varId) {
    Context& context = getContext(contextId);
    Context::iterator varIt = context.find(varId);
    if (varIt == context.end()) {
        throw InterpretationException("Var is not initialized");
    }
}

ContextManager::Context& ContextManager::getContext(uint16_t contextId) {
    checkContext(contextId);
    assert(_contextById.at(contextId).size() > 0);
    return _contextById.at(contextId).back();
}

void ContextManager::addContext(uint16_t contextId) {
    _contextById[contextId].push_back(Context());
}

void ContextManager::removeContext(uint16_t contextId) {
    checkContext(contextId);
    _contextById[contextId].pop_back();
    if (_contextById.at(contextId).size() == 0) {
        _contextById.erase(contextId);
    }
}

void ContextManager::setCurrentContextId(uint16_t contextId) {
    _currentContextId = contextId;
}

uint16_t ContextManager::getCurrentContextId() {
    return _currentContextId;
}

double ContextManager::loadDoubleFromCtxVar(uint16_t contextId, uint16_t varId) {
    try {
        checkCtxVar(contextId, varId);
    }
    catch(InterpretationException& exception) {
        double defaultValue = 0.0;
        getContext(contextId)[varId] = ContextVar(defaultValue);
    }
    return getContext(contextId).at(varId).doubleValue();
}

double ContextManager::loadDoubleFromVar(uint16_t varId) {
    return loadDoubleFromCtxVar(getCurrentContextId(), varId);
}

int64_t ContextManager::loadIntFromCtxVar(uint16_t contextId, uint16_t varId) {
    try {
        checkCtxVar(contextId, varId);
    }
    catch(InterpretationException& exception) {
        int64_t defaultValue = 0;
        getContext(contextId)[varId] = ContextVar(defaultValue);
    }
    return getContext(contextId).at(varId).intValue();
}

int64_t ContextManager::loadIntFromVar(uint16_t varId) {
    return loadIntFromCtxVar(getCurrentContextId(), varId);
}

uint16_t ContextManager::loadStringIdFromCtxVar(uint16_t contextId, uint16_t varId) {
    try {
        checkCtxVar(contextId, varId);
    }
    catch(InterpretationException& exception) {
        uint16_t defaultValue = 0;
        getContext(contextId)[varId] = ContextVar(defaultValue);
    }
    return getContext(contextId).at(varId).stringIdValue();
}

uint16_t ContextManager::loadStringIdFromVar(uint16_t varId) {
    return loadStringIdFromCtxVar(getCurrentContextId(), varId);
}

void ContextManager::storeDoubleToCtxVar(uint16_t contextId, uint16_t varId, double dvalue) {
    Context& context = getContext(contextId);
    context[varId].setDouble(dvalue);
}

void ContextManager::storeDoubleToVar(uint16_t varId, double dvalue) {
    storeDoubleToCtxVar(getCurrentContextId(), varId, dvalue);
}

void ContextManager::storeIntToCtxVar(uint16_t contextId, uint16_t varId, int64_t ivalue) {
    Context& context = getContext(contextId);
    context[varId].setInt(ivalue);
}

void ContextManager::storeIntToVar(uint16_t varId, int64_t ivalue) {
    storeIntToCtxVar(getCurrentContextId(), varId, ivalue);
}

void ContextManager::storeStringIdToCtxVar(uint16_t contextId, uint16_t varId, uint16_t svalue) {
    Context& context = getContext(contextId);
    context[varId].setStringId(svalue);
}

void ContextManager::storeStringIdToVar(uint16_t varId, uint16_t svalue) {
    storeStringIdToCtxVar(getCurrentContextId(), varId, svalue);
}

}
