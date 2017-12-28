#pragma once

#include <map>
#include <memory>
#include "mathvm.h"

using namespace mathvm;

class TranslatedScope {
    typedef map<string, uint16_t> NameMapVarId;
    TranslatedScope* parent;
    BytecodeFunction* function;
    shared_ptr<NameMapVarId> nameToVarId;
    vector<Var> variables;
    const uint16_t id;

public:
    friend class Context;
    static const uint16_t INVALID_VAR_ID = static_cast<uint16_t>(-1);
    static const uint16_t INVALID_SCOPE_ID = static_cast<uint16_t>(-1);

    TranslatedScope(TranslatedScope* parent, BytecodeFunction* function)
            : parent(parent)
            , function(function)
            , nameToVarId(new NameMapVarId())
            , id (function->id())
    {}

    uint16_t declareVariable(const string& name, VarType type) {
        variables.emplace_back(type, name);
        const uint16_t varId = static_cast<uint16_t>(variables.size() - 1);
        nameToVarId->insert(std::make_pair(name, varId));
        return varId;
    }

    uint16_t getVariableId(const string& name) {
        auto it = nameToVarId->find(name);
        if (it != nameToVarId->end()) {
            return it->second;
        }

        // search in parent scope
        if (parent == nullptr) {
            return INVALID_VAR_ID;
        }

        uint16_t varId = parent->getVariableId(name);
        if (varId != INVALID_VAR_ID) {
            return varId;
        }

        return INVALID_VAR_ID;
    }

    uint16_t getNumVariables() {
        return static_cast<uint16_t>(nameToVarId->size());
    }

    BytecodeFunction *getFunction() {
        return function;
    }

    TranslatedScope *getParent() {
        return parent;
    }

    uint16_t getId() const {
        return id;
    }

    uint16_t getScopeId(const string &varName) {
        auto iterator = nameToVarId->find(varName);
        if (iterator != nameToVarId->end()) {
            return id;
        }

        // search in parent scope
        if (parent == nullptr) {
            return INVALID_SCOPE_ID;
        }

        uint16_t varId = parent->getScopeId(varName);
        if (varId != INVALID_SCOPE_ID) {
            return varId;
        }

        return INVALID_SCOPE_ID;
    }
};
