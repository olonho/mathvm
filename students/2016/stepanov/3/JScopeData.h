#ifndef VM_JSCOPEDATA_H
#define VM_JSCOPEDATA_H

#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"
#include "../../../../libs/asmjit/x86.h"
#include "ScopeData.h"
#include "AScopeData.h"
#include <string>
#include <map>
#include <memory>

namespace mathvm {
    static uint16_t jscope_count = 0;
    static int64_t functions_count = 0;

    struct AsmScopeVariable {
        asmjit::X86Var *var;
        VarType type;
        bool isPointer;
        bool readOnly;

        AsmScopeVariable(asmjit::X86Var *var, VarType type, bool isPointer, bool readOnly);

        AsmScopeVariable(const AsmScopeVariable &other);

        AsmScopeVariable(AsmScopeVariable &&other);

        AsmScopeVariable &operator=(const AsmScopeVariable &right);

        AsmScopeVariable &operator=(AsmScopeVariable &&right);
    };

    extern asmjit::Label doubleZero;

    class JScopeData {
    private:
        std::map<std::string, std::pair<int64_t, AstFunction *>> functions;
    public:
        std::shared_ptr<JScopeData> parent;
        std::map<std::string, mathvm::AsmScopeVariable *> variables;
        AstFunction *containedFunction = nullptr;
        asmjit::X86Compiler *compiler = nullptr;
        NodeScopeData *currentNodeScope = nullptr;
        asmjit::X86Var *returnVariable = nullptr;
        asmjit::Label *returnLabel = nullptr;
        uint16_t jscope_id = 0;
        VarType topType = VT_VOID;
    public:
        JScopeData(AstFunction *containedFunction) : containedFunction(containedFunction) {
            jscope_id = scope_count++;
        }

        JScopeData(shared_ptr<JScopeData> parent) {
            this->parent = parent;
            if (parent.get() != nullptr) {
                containedFunction = parent->containedFunction;
                compiler = parent->compiler;
                topType = parent->topType;
                returnVariable = parent->returnVariable;
                returnLabel = parent->returnLabel;
                jscope_id = parent->jscope_id;
            } else {
                if (scope_count == UINT16_MAX) {
                    throw new std::runtime_error("you use all 65536 variables in current scope");
                }
                jscope_id = jscope_count++;
            }
        }

        JScopeData(AstFunction *containedFunction, asmjit::X86Compiler *compiler, shared_ptr<JScopeData> parent)
                : parent(parent),
                  containedFunction(containedFunction),
                  compiler(compiler) {
            returnVariable = parent->returnVariable;
            returnLabel = parent->returnLabel;
            topType = parent->topType;
            if (scope_count == UINT16_MAX) {
                throw new std::runtime_error("you use all 65536 variables in current scope");
            }
            jscope_id = jscope_count++;
        }

        JScopeData(AstFunction *containedFunction, shared_ptr<JScopeData> parent)
                : JScopeData(containedFunction, nullptr, parent) {
        }

        AstFunction *lookupFunctionByName(const std::string &name, int64_t &function_id, bool userParent = true) {
            auto target = functions.find(name);
            if (target == functions.end()) {
                if (!userParent) {
                    function_id = -1;
                    return nullptr;
                } else if (parent == nullptr) {
                    function_id = -1;
                    return nullptr;
                } else return parent->lookupFunctionByName(name, function_id);
            } else {
                auto result = target->second;
                function_id = result.first;
                return result.second;
            }
        }

        AsmScopeVariable *lookupVariableInfo(const std::string &name) {
            auto target = variables.find(name);
            if (target == variables.end()) {
                if (parent == nullptr) {
                    return nullptr;
                } else return parent->lookupVariableInfo(name);
            } else {
                return target->second;
            }
        }


        const ManagedVariable *lookupManagedVariable(const std::string &name) {
            auto target = currentNodeScope->captured.find(ManagedVariable(name, VT_INVALID));
            if (target == currentNodeScope->captured.end()) {
                return nullptr;
            } else {
                return &(*target);
            }
        }

        inline asmjit::X86Var *addVariable(AstVar *var, bool firstTime) {
            return addVariable(var->name(), var->type(), firstTime);
        }

        asmjit::X86Var *
        addVariable(const string &name, VarType vt, bool firstTime, bool isPointer = false, bool readOnly = false) {
            asmjit::X86Var *targetVal;
            switch (vt) {
                case VT_DOUBLE:
                    targetVal = new asmjit::X86XmmVar();
                    if (isPointer) {
                        *targetVal = compiler->newIntPtr();
                    } else {
                        *targetVal = compiler->newXmm();
                        if (firstTime) {
                            compiler->movq(*(asmjit::X86XmmVar *) targetVal, asmjit::x86::ptr(doubleZero));
                        }
                    }
                    break;
                case VT_STRING:
                case VT_INT: {
                    targetVal = new asmjit::X86GpVar();
                    if (isPointer) {
                        *targetVal = compiler->newIntPtr();
                    } else {
                        *targetVal = compiler->newInt64();
                        if (firstTime) {
                            compiler->mov(*(asmjit::X86GpVar *) targetVal, 0);
                        }
                    }
                }
                    break;
                default:
                    assert(false);
            }
            variables[name] = new AsmScopeVariable(targetVal, vt, isPointer, readOnly);
            return targetVal;
        }

        void
        registerVariable(const string &name, asmjit::X86Var *targetVal, VarType type, bool isPointer, bool readOnly) {
            variables[name] = new AsmScopeVariable(targetVal, type, isPointer, readOnly);
        }

        void addFunction(AstFunction *function) {
            functions.insert(make_pair(function->name(), std::make_pair(functions_count++, function)));
        }


    };

}

#endif //VM_SCOPEDATA_H
