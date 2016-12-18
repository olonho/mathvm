//
// Created by user on 11/12/16.
//

#ifndef VM_BYTECODE_PRINTER_INTERPRETER_H
#define VM_BYTECODE_PRINTER_INTERPRETER_H

#include "StackItem.h"
#include "InterScope.h"
#include <cstring>
#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"
#include "../../../../libs/asmjit/asmjit.h"
#include <set>



namespace mathvm {
    extern std::vector<StackItem> variables;
    extern std::stack<size_t> scopeOffsets[UINT16_MAX];
    extern asmjit::JitRuntime runtime;

    typedef int64_t (*nativePtr)(int64_t *);
    typedef double (*nativeDoublePtr)(int64_t *);

    typedef void (*nativeVoidPtr)(int64_t *);

    extern int64_t nativeLinks[UINT16_MAX];
    extern nativePtr nativeFunctions[UINT16_MAX];
    extern std::vector<StackItem> interpreterStack;

    extern const int MAX_RARGS;
    extern const int MAX_XMM;
    extern asmjit::X86GpReg rRegisters[];
    extern asmjit::X86XmmReg xmmRegisters[];

    inline int countRegisterArguments(const Signature &signature, int &rArgsCnt, int &xmmCnt) {
        for (size_t i = 1; i < signature.size(); ++i) {
            rArgsCnt += signature[i].first != VT_DOUBLE;
            xmmCnt += signature[i].first == VT_DOUBLE;
        }
        rArgsCnt = std::min(rArgsCnt, MAX_RARGS);
        xmmCnt = std::min(xmmCnt, MAX_XMM);
        return rArgsCnt + xmmCnt;
    }


    nativePtr buildNativeFunction(const Signature &signature, const void *call);
    nativePtr buildNativeProxy(const Signature &signature, const void *call);

    class BytecodeRFInterpreterCode : public Code {
    private:
        size_t variablesOffset;
        InterScope *is = nullptr;
        std::map<std::string, uint16_t> topLevelVariables;

        bool evaluateThis(Instruction instr);

        uint16_t emptyString();

    public:
        ~BytecodeRFInterpreterCode() {
            delete (is);
        }

        BytecodeRFInterpreterCode() {
            interpreterStack.reserve(100);
            memset(nativeLinks, 0, sizeof(int64_t) * UINT16_MAX);
            memset(nativeFunctions, 0, sizeof(nativePtr) * UINT16_MAX);
        }

        void saveVariablesNamesForOuterExecution(std::map<std::string, uint16_t> &topScopeVariables) {
            topLevelVariables = topScopeVariables;
        }

        virtual Status *execute(vector<Var *> &vars) override {
            BytecodeFunction *topFunction = (BytecodeFunction *) functionByName(AstFunction::top_name);
            is = new InterScope(topFunction);
            variablesOffset = is->variableOffset;

            for (size_t i = 0; i < vars.size(); ++i) {
                auto it = topLevelVariables.find(vars[i]->name());
                if (it != topLevelVariables.end()) {
                    switch (vars[i]->type()) {
                        case VT_DOUBLE:
                            variables[variablesOffset + it->second] = vars[i]->getDoubleValue();
                            break;
                        case VT_INT:
                            variables[variablesOffset + it->second] = vars[i]->getIntValue();
                            break;
                        case VT_STRING:
                            variables[variablesOffset + it->second] = makeStringConstant(vars[i]->getStringValue());
                            break;
                        default:
                            break;
                    }
                }
            }

            Instruction nextInstruction;
            while ((nextInstruction = is->next()) != BC_STOP) {
                if (!evaluateThis(nextInstruction)) {
                    break;
                }
            }

            for (size_t i = 0; i < vars.size(); ++i) {
                auto it = topLevelVariables.find(vars[i]->name());
                if (it != topLevelVariables.end()) {
                    switch (vars[i]->type()) {
                        case VT_DOUBLE:
                            vars[i]->setDoubleValue(variables[variablesOffset + it->second].value.doubleValue);
                            break;
                        case VT_INT:
                            vars[i]->setIntValue(variables[variablesOffset + it->second].value.intValue);
                            break;
                        case VT_STRING:
                            vars[i]->setStringValue((char *) getStringConstantPtrById(
                                    variables[variablesOffset + it->second].value.stringIdValue));
                            break;
                        default:
                            break;
                    }
                }
            }
#ifdef MY_DEBUG
            std::cout << "Stack size on out: " << interpreterStack.size() << std::endl;
#endif

            return is ? is->status : Status::Ok();
        }

        int64_t getStringConstantPtrById(uint16_t id);

        void registerStringConstantPtrById(uint16_t id, int64_t ptr);
    };
}

#endif //VM_BYTECODE_PRINTER_INTERPRETER_H
