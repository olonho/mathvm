#ifndef VM_AF_3_ASCOPEDATA_H
#define VM_AF_3_ASCOPEDATA_H


#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"
#include "../../../../libs/asmjit/x86.h"
#include "ScopeData.h"
#include "VmException.h"
#include <string>
#include <map>
#include <set>
/*#include <bits/unordered_map.h>
#include <bits/unordered_set.h>
#include <unordered_map>
#include <unordered_set>*/

namespace mathvm {
    static uint16_t annotated_scope_count = 0;

    struct ManagedVariable {
        std::string name;
        VarType type;
        bool isPointer;

        ManagedVariable(const string &name, VarType type, bool isPointer = false);

        ManagedVariable(const ManagedVariable &other);

        bool operator==(const ManagedVariable &rhs) const;

        bool operator!=(const ManagedVariable &rhs) const;

        inline ManagedVariable getNotPointed() const {
            return ManagedVariable(name, type, false);
        }

        inline ManagedVariable getPointed() const {
            return ManagedVariable(name, type, true);
        }

        bool operator<(const ManagedVariable &rhs) const;

        bool operator>(const ManagedVariable &rhs) const;

        bool operator<=(const ManagedVariable &rhs) const;

        bool operator>=(const ManagedVariable &rhs) const;
    };


/*    template <>
    struct hash<ManagedVariable>
    {
        std::size_t operator()(const ManagedVariable& k) const
        {
            using std::size_t;
            using std::hash;
            using std::string;

            // Compute individual hash values for first,
            // second and third and combine them using XOR
            // and bit shifting:

            return hash<string>()(k.name);
        }
    };*/

    //std::unordered_map<ManagedVariable, uint64_t> writed;

    struct NodeScopeData {
        /*std::set<ManagedVariable> writed;
        std::set<ManagedVariable> readed;*/
        std::set<ManagedVariable> captured;
        bool hasInternalCalls = false;
    };

    extern size_t total_captured_count;

    extern std::map<AstFunction *, NodeScopeData *> functionsNodeScopeData;

    class AScopeData {
    private:
        std::map<std::string, AstFunction *> functions;
        std::map<std::string, VariableRF> variableInfo;
    public:
        NodeScopeData *currentNodeScope = new NodeScopeData();
        AScopeData *parent = nullptr;
        std::map<std::string, uint16_t> variables;
        AstFunction *containedFunction = nullptr;
        size_t current_function_id;
        uint16_t scope_id = 0;
        uint16_t max_id = 0;
    public:
        AScopeData(AstFunction *containedFunction) : containedFunction(containedFunction) {
            scope_id = annotated_scope_count++;
        }

        AScopeData(AScopeData *parent) : parent(parent) {
            if (parent != nullptr) {
                containedFunction = parent->containedFunction;
                max_id = parent->max_id;
                current_function_id = parent->current_function_id;
                scope_id = parent->scope_id;
            } else {
                if (annotated_scope_count == UINT16_MAX) {
                    throw new std::runtime_error("you use all 65536 variables in current scope");
                }
                scope_id = annotated_scope_count++;
            }
        }

        AScopeData(AstFunction *containedFunction, asmjit::X86Compiler *compiler, AScopeData *parent)
                : parent(parent),
                  containedFunction(containedFunction) {
            if (annotated_scope_count == UINT16_MAX) {
                throw new std::runtime_error("you use all 65536 variables in current scope");
            }
            current_function_id = parent->current_function_id;
            scope_id = annotated_scope_count++;
        }

        AScopeData(AstFunction *containedFunction, AScopeData *parent)
                : AScopeData(containedFunction, nullptr, parent) {
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

        AstFunction *lookupFunctionByName(const std::string &name, bool userParent = true) {
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

        void registerVariableLoad(const ManagedVariable &var, bool top = true) {
            auto target = variableInfo.find(var.name);
            if (target == variableInfo.end()) {
                if (parent == nullptr) {
                    throw new VmException("cannot find variable" + var.name, 0);
                } else {
                    parent->registerVariableLoad(var, false);
                    const pair<set<mathvm::ManagedVariable>::iterator, bool> &pair = currentNodeScope->captured.insert(var.getPointed());
                    total_captured_count += pair.second;
                }
            }
            /*if (top) {
                currentNodeScope->readed.insert(var.getNotPointed());
            } else {
                currentNodeScope->readed.insert(var.getPointed());
            }*/

        }

        void registerVariableStore(const ManagedVariable &var, bool top = true) {
            auto target = variableInfo.find(var.name);
            if (target == variableInfo.end()) {
                if (parent == nullptr) {
                    throw new VmException("cannot find variable" + var.name, 0);
                } else {
                    parent->registerVariableStore(var, false);
                    total_captured_count += currentNodeScope->captured.insert(var.getPointed()).second;
                }
            }
            /*if (top) {
                currentNodeScope->writed.insert(var.getNotPointed());
            } else {
                currentNodeScope->writed.insert(var.getPointed());
            }*/
        }

        void registerVariableCapture(const ManagedVariable &var, bool top = true) {
            auto target = variableInfo.find(var.name);
            if (target == variableInfo.end()) {
                if (parent == nullptr) {
                    throw new VmException("cannot find variable: " + var.name, 0);
                } else {
                    parent->registerVariableCapture(var, false);
                    total_captured_count += currentNodeScope->captured.insert(var.getPointed()).second;
                }
            }
        }

        uint16_t addVariable(AstVar *var) {
            if (variables.find(var->name()) == variables.end()) {
                variables[var->name()] = max_id;
                variableInfo.insert(make_pair(var->name(), VariableRF(scope_id, max_id)));
            }
            return max_id++;
        }

        void addFunction(AstFunction *function) {
            functions.insert(make_pair(function->name(), function));
        }

        std::string getFunctionName(AstFunction *bf) {
            if (parent == nullptr) {
                return bf->name();
            } else if (bf == containedFunction) {
                return parent->getFunctionName(bf);
            } else {
                return parent->getFunctionName(containedFunction) + "$" + bf->name();
            }
        }

        void saveFunctionInfo() {
            functionsNodeScopeData[containedFunction] = currentNodeScope;
        }

    };

}


#endif //VM_AF_3_ASCOPEDATA_H
