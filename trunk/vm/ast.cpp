#include "ast.h"

#include <iostream>

namespace mathvm {

#define VISIT_FUNCTION(type, name)           \
    void type::visit(AstVisitor* visitor) {  \
        visitor->visit##type(this);          \
    }

FOR_NODES(VISIT_FUNCTION)

#undef VISIT_FUNCTION

const string AstFunction::top_name = "<top>";
const string AstFunction::invalid = "<invalid>";

const string& AstFunction::name() const {
    if (isTop()) {
        return top_name;
    }
    return _function->name();
}

VarType AstFunction::returnType() const {
    if (isTop()) {
        return VT_VOID;
    }
    return _function->returnType();
}

VarType AstFunction::parameterType(uint32_t index) const {
    if (isTop()) {
        return VT_VOID;
    }
    return _function->parameterType(index);
}

const string& AstFunction::parameterName(uint32_t index) const {
    if (isTop()) {
        return invalid;
    }
    return _function->parameterName(index);
}


uint32_t AstFunction::parametersNumber() const {
    if (isTop()) {
        return 0;
    }
    return _function->parametersNumber();
}

BlockNode::~BlockNode() {
    delete _scope;
}

Scope::~Scope() {
    VarMap::iterator it = _vars.begin();

    for (; it != _vars.end(); ++it) {
        delete (*it).second;
    }
}

void Scope::declareVariable(const string& name, VarType type) {
    _vars[name] = new AstVar(name, type, this);
}

void Scope::declareFunction(FunctionNode* node) {
  _functions[node->name()] = new AstFunction(node, this);
}

AstVar* Scope::lookupVariable(const string& name) {
    AstVar* result = 0;
    VarMap::iterator it = _vars.find(name);
    if (it != _vars.end()) {
        result = (*it).second;
    }

    if (!result && _parent) {
        result = _parent->lookupVariable(name);
    }

    return result;
}

AstFunction* Scope::lookupFunction(const string& name) {
    AstFunction* result = 0;
    FunctionMap::iterator it = _functions.find(name);
    if (it != _functions.end()) {
        result = (*it).second;
    }

    if (!result && _parent) {
        result = _parent->lookupFunction(name);
    }

    return result;
}

bool Scope::VarIterator::hasNext() {
    if (_it != _scope->_vars.end()) {
        return true;
    }
    if (!_includeOuter) {
        return false;
    }
    if (!_scope->_parent) {
        return false;
    }
    _scope = _scope->_parent;
    _it = _scope->_vars.begin();

    return hasNext();
}

AstVar* Scope::VarIterator::next() {
    if (!hasNext()) {
        return 0;
    }

    AstVar* next = (*_it).second;
    _it++;
    return next;
}


bool Scope::FunctionIterator::hasNext() {
    if (_it != _scope->_functions.end()) {
        return true;
    }
    if (!_includeOuter) {
        return false;
    }
    if (!_scope->_parent) {
        return false;
    }
    _scope = _scope->_parent;
    _it = _scope->_functions.begin();

    return hasNext();
}

AstFunction* Scope::FunctionIterator::next() {
    if (!hasNext()) {
        return 0;
    }

    AstFunction* next = (*_it).second;
    _it++;
    return next;
}

}
