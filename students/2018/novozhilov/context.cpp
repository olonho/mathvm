#include <utility>
#include "include/context.h"

using namespace mathvm;

Context::Context(BytecodeFunction *function, Context *parentContext, Scope *scope)
    : _function(function)
    , _parentContext(parentContext)
    , _variables(function->localsNumber()) {
    if (scope != nullptr) {
        Scope::VarIterator iterator(scope);
        while (iterator.hasNext()) {
            AstVar *astVar = iterator.next();
            addVar(new Var(astVar->type(), astVar->name()));
        }
    }
}

uint16_t Context::addVar(Var *var) {
    auto index = static_cast<uint16_t>(_variables.size());
    StackValue value;
    switch (var->type()) {
        case VT_INT:
            value = StackValue(var->getIntValue());
            break;
        case VT_DOUBLE:
            value = StackValue(var->getDoubleValue());
            break;
        case VT_STRING:
            value = StackValue(var->getStringValue());
            break;
        default:
            throw std::runtime_error("invalid type");
    }
    _variables.push_back(value);
    _variableById[var->name()] = index;
    return index;
}

bool Context::containsVariable(string name) {
    return _variableById.find(name) != _variableById.end();
}

Context *Context::getParentContext() {
    return _parentContext;
}

uint16_t Context::getContextId() {
    return _function->id();
}

Bytecode *Context::getBytecode() {
    return _function->bytecode();
}

uint16_t Context::addVar(AstVar *var) {
    return addVar(new Var(var->type(), var->name()));
}

uint16_t Context::getVarId(string name) {
    return _variableById[name];
}

uint16_t Context::getVarsCount() {
    return static_cast<uint16_t>(_variables.size());
}

