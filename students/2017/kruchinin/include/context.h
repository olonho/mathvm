#pragma once

#include <stack>
#include "scope.h"
#include "utils.h"

using namespace details;

class Context {
private:
    typedef vector<StackVar> VarHolder;
    typedef stack<VarHolder> CurContextHolder;
    vector<CurContextHolder> context;

public:
    Context(vector<TranslatedScope*> scopes) {
        context = vector<CurContextHolder>(scopes.size());

        for (auto scope: scopes) {
            VarHolder varHolder = VarHolder(scope->variables.size());
            for (auto var: scope->variables) {
                switch (var.type()) {
                    case VT_INT:
                        varHolder.push_back(StackVar((int64_t) 0));
                        break;
                    case VT_DOUBLE:
                        varHolder.push_back(StackVar((double) 0.));
                        break;
                    case VT_STRING:
                        varHolder.push_back(StackVar((uint16_t) 0));
                        break;
                    default:
                        assert(false);
                }
            }
            CurContextHolder contextHolder;
            contextHolder.push(varHolder);
            context[scope->getId()] = contextHolder;
        }

        for (auto scope: scopes) {
            delete scope;
        }
    }

    void createContext(BytecodeFunction* function) {
        const uint16_t scopeId = function->id();
        const vector<StackVar> &lastContext = context[scopeId].top();
        context[scopeId].push(lastContext);
    }

    void popContext(BytecodeFunction* function) {
        const uint16_t scopeId = function->id();
        assert(!context[scopeId].empty() && "call stack corruption");
        context[scopeId].pop();
    }

    StackVar& getVar(uint16_t scopeId, uint16_t varId) {
        return context[scopeId].top()[varId];
    }

    void setVarDouble(uint16_t scopeId, uint16_t varId, double value) {
        context[scopeId].top()[varId].setDoubleValue(value);
    }

    void setVarInt(uint16_t scopeId, uint16_t varId, int64_t value) {
        context[scopeId].top()[varId].setIntValue(value);
    }

    void setVarString(uint16_t scopeId, uint16_t varId, uint16_t value) {
        context[scopeId].top()[varId].setUInt16Value(value);
    }
};
