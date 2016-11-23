#pragma once

#include <map>
#include <string.h>

#include "mathvm.h"
#include "ast.h"

namespace mathvm {

struct VarDescr {
    uint16_t id;
    uint16_t contextId;

    VarDescr(uint16_t id, uint16_t contextId) :
            id(id),
            contextId(contextId) {}
};

class Context {
    uint16_t id;
    BytecodeFunction* bFunction;
    Context* parent;
    std::map<std::string, uint16_t> variables;
    VarType tosType;
public:
    Context(BytecodeFunction* bf, Scope* scope, Context* parent = NULL) :
            bFunction(bf),
            parent(parent) {
        id = 0;
        if (parent != NULL) {
            id = parent->getId() + 1;
        }

        Scope::VarIterator it = Scope::VarIterator(scope);
        while(it.hasNext()) {
            variables[it.next()->name()] = variables.size() - 1;
        }
    }

    uint16_t getId() {
        return id;
    }

    BytecodeFunction* getBf() {
        return bFunction;
    }

    uint16_t getVarsNum() {
        return variables.size();
    }

    Context* getParent() {
        return parent;
    }

    Bytecode* bc() {
        return getBf()->bytecode();
    }

    void setTOSType(VarType t) {
        tosType = t;
    }

    VarType getTOSType() {
        return tosType;
    }

    VarDescr getVarDescr(std::string const & var_name) {
        if (variables.find(var_name) == variables.end()) {
            if (parent == NULL) {
                throw new std::runtime_error("Variable not found: " + var_name);
            }
            return parent->getVarDescr(var_name);
        }
        return VarDescr(variables[var_name], id);
    }

    bool containsVar(std::string const & var_name) {
        if (variables.find(var_name) == variables.end()) {
            if (parent == NULL) {
                return false;
            }
            return parent->containsVar(var_name);
        }
        return true;
    }

    void addVar(AstVar const * var) {
        if (variables.size() > (((unsigned) 2 >> 16) - 1)) {
            throw new std::runtime_error("Too much variables");
        }
        variables[var->name()] = variables.size() - 1;
    }
};

}