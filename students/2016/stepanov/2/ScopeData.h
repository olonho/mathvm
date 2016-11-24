//
// Created by user on 10/29/16.
//

#ifndef VM_SCOPEDATA_H
#define VM_SCOPEDATA_H

#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"
#include <string>
#include <map>

namespace mathvm {
    class VariableRF {
    public:
        uint16_t variable_scope;
        uint16_t variable_id;

        VariableRF(uint16_t variable_scope, uint16_t variable_id) : variable_scope(variable_scope),
                                                                    variable_id(variable_id) {}
    };

    static uint16_t scope_count = 0;

    class ScopeData {
    private:
        ScopeData *parent = nullptr;
        std::map<std::string, uint16_t> variables;
        std::map<std::string, BytecodeFunction *> functions;
        std::map<std::string, VariableRF> variableInfo;
        uint16_t maxNestedStack = 0;
    public:
        BytecodeFunction *containedFunction = nullptr;
        uint16_t scope_id = 0;
        uint16_t max_id = 0;
        VarType topType = VT_VOID;
    public:
        ScopeData(BytecodeFunction *containedFunction) : containedFunction(containedFunction) {
            scope_id = scope_count++;
        }

        ScopeData(ScopeData *parent) : parent(parent) {
            if (parent != nullptr) {
                containedFunction = parent->containedFunction;
                max_id = parent->max_id;
                topType = parent->topType;
                scope_id = parent->scope_id;
            } else {
                if (scope_count == UINT16_MAX) {
                    throw new std::runtime_error("you use all 65536 variables in current scope");
                }
                scope_id = scope_count++;
            }
        }

        ScopeData(BytecodeFunction *containedFunction, ScopeData *parent) : parent(parent),
                                                                            containedFunction(containedFunction) {
            if (scope_count == UINT16_MAX) {
                throw new std::runtime_error("you use all 65536 variables in current scope");
            }
            scope_id = scope_count++;
        }

        const VariableRF *lookupVariableInfo(const std::string &name) {
            auto target = variableInfo.find(name);
            if (target == variableInfo.end()) {
                if (parent == nullptr) {
                    return nullptr;
                } else return parent->lookupVariableInfo(name);
            } else {
                return &(target->second);
            }
        }

        BytecodeFunction *lookupFunctionByName(const std::string &name, bool userParent = true) {
            auto target = functions.find(name);
            if (target == functions.end()) {
                if (!userParent) {
                    return nullptr;
                } else if (parent == nullptr) {
                    return nullptr;
                } else return parent->lookupFunctionByName(name);
            } else {
                return target->second;
            }
        }

        void updateNestedStack(uint16_t size) {
            maxNestedStack = std::max(maxNestedStack, size);
        }

        uint16_t addVariable(AstVar *var) {
            variables[var->name()] = max_id;
            variableInfo.insert(make_pair(var->name(), VariableRF(scope_id, max_id)));
            return max_id++;
        }

        void addFunction(BytecodeFunction *function) {
            functions.insert(make_pair(function->name(), function));
        }

        uint16_t getCountVariablesInScope() {
            return (uint16_t) variables.size() + maxNestedStack;
        }

    };

}

#endif //VM_SCOPEDATA_H
