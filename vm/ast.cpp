#include "ast.h"

#include <iostream>

namespace mathvm {

#define VISIT_FUNCTION(type, name)           \
    void type::visit(AstVisitor* visitor) {  \
        visitor->visit##type(this);          \
    }

FOR_NODES(VISIT_FUNCTION)

#undef VISIT_FUNCTION

BlockNode::~BlockNode() {
    delete _scope;
}

Scope::~Scope() {
    VarMap::iterator it = _vars.begin();
    
    for (; it != _vars.end(); ++it) {
        delete (*it).second;
    }
}

void Scope::declare(const string& name, VarType type) {
    _vars[name] = new AstVar(name, type, this);
}

AstVar* Scope::lookup(const string& name) {
    AstVar* result = 0;
    VarMap::iterator it = _vars.find(name);
    if (it != _vars.end()) {
        result = (*it).second;
    }

    if (!result && _parent) {
        result = _parent->lookup(name);
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

}
