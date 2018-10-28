//
// Created by jetbrains on 23.10.18.
//

#include "include/context.h"

using namespace mathvm;

Context::Context(BytecodeFunction *function, Context *parentContext, Scope *scope)
    : _function(function)
    , _parentContext(parentContext)
    , _bytecodePosition(0)
    , _scopeId(parentContext ? parentContext->getContextId() + static_cast<uint16_t>(1) : static_cast<uint16_t >(0))
    , _variables(scope->variablesCount()) {
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
    _variables.push_back(var);
    _variableById[var->name()] = index;
    return index;
}

Var *Context::getVarById(uint16_t index) {
    if (index >= _variables.size()) {
        throw std::runtime_error("index out of bounds");
    }
    return _variables[index];
}

Var *Context::getVar(string name) {
    if (_variableById.find(name) == _variableById.end()) {
        throw std::runtime_error("no var named " + name);
    }
    return _variables[_variableById[name]];
}

Context *Context::getParentContext() {
    return _parentContext;
}

uint32_t Context::getBytecodePosition() {
    return _bytecodePosition;
}

uint16_t Context::getContextId() {
    return _scopeId;
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

BytecodeFunction *Context::getFunction() {
    return _function;
}

VarType Context::getTypeOnStackTop() {
    return _typeOnStackTop;
}

void Context::setTypeOnStackTop(VarType type) {
    _typeOnStackTop = type;
}

uint16_t Context::getVarsCount() {
    return _variables.size();
}

uint16_t Context::addVar(string const &name, VarType type) {
    return addVar(new Var(type, name));
}



