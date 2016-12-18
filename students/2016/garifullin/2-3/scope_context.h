#ifndef SCOPE_CONTEXT_H
#define SCOPE_CONTEXT_H

#include <map>
#include <string>
#include <limits>
#include <iostream>

#include "mathvm.h"
#include "translator_exception.h"

namespace mathvm {

class VarContext {
public:
    const uint16_t var_id;
    const uint16_t context_id;

    VarContext(uint16_t id, uint16_t context_id) :
        var_id(id),
        context_id(context_id) {

    }
};


class ScopeContext {

    BytecodeFunction* _byte_func;
    Scope* _scope;
    ScopeContext* _parent;
    VarType _TOS_type;
    std::map<std::string, uint16_t> _vars;
    uint16_t _id;

public:
    ScopeContext(BytecodeFunction* bf, Scope* scope, ScopeContext* p = nullptr):
        _byte_func(bf),
        _scope(scope),
        _parent(p) {

        if (p != nullptr) {
            _id = p->getId() + 1;
        } else {
            _id = 0;
        }

        Scope::VarIterator it = Scope::VarIterator(_scope);
        while(it.hasNext()) {
            AstVar *var = it.next();
            size_t i = _vars.size();
            _vars[var->name()] = i;
        }

    }
    ~ScopeContext() {
    }

    bool containsVar(std::string const & var_name) {
        if (_vars.find(var_name) == _vars.end()) {
            if (_parent == nullptr) {
                return false;
            } else {
                return _parent->containsVar(var_name);
            }

        } else {
            return true;
        }
    }


    void addVar(AstVar const * var) {
        if (_vars.size() > std::numeric_limits<uint16_t>::max()) {
            throw TranslatorException("Too many variables");
        }
        size_t i = _vars.size();
        _vars[var->name()] = i;
    }

    void setTosType(VarType t) {
        _TOS_type = t;
    }

    VarType getTosType() {
        return _TOS_type;
    }

    VarContext getVarContext(std::string const & var_name) {
        if (_vars.find(var_name) == _vars.end()) {
            if (_parent == nullptr) {
                throw TranslatorException("Var not found: " + var_name);
            } else {
                return _parent->getVarContext(var_name);
            }
        } else {
            return VarContext(_vars[var_name], _id);
        }
    }

    uint16_t getId() {
        return _id;
    }


    BytecodeFunction* getByteFunc() {
        return _byte_func;
    }

    uint16_t getLocalsNum() {
        return _vars.size();
    }

    ScopeContext* getParent() {
        return _parent;
    }

    Scope* getScope() {
        return _scope;
    }

};

}

#endif // SCOPE_CONTEXT_H
