#include "SimpleInterpreter.hpp"
#include "asmjit/asmjit.h"
#include "Errors.hpp"

using namespace asmjit;

namespace mathvm {
    static uint32_t asmjitTypeFromMathVMType(VarType type) {
        switch (type) {
            case VT_DOUBLE:
                return kX86VarTypeXmmSd;
            case VT_INT:
                return kVarTypeInt64;
            case VT_STRING:
                return kVarTypeIntPtr;
            default:
                throw InterpretationError(string("Wrong NativeFunction type while converting to AsmJitType") + typeToName(type));
        }
    }

    void setXmmVariable(X86Compiler &c, XmmVar &v, double d) {
        X86GpVar temp = c.newGpVar();
        c.mov(temp, *(reinterpret_cast<uint64_t *>(&d)));
        c.movq(v, temp.m());
        c.unuse(temp);
    }

    void SimpleInterpreter::callNative(uint16_t id) {
        const Signature *signature;
        const std::string *name;
        void *nativeFunctionAddress = (void *) nativeById(id, &signature, &name);
        if (!nativeFunctionAddress) {
            throw InterpretationError("Native function not found");
        }

        JitRuntime runtime;
        X86Compiler compiler(&runtime);

        FuncBuilderX mainFunctionPrototype;
        FuncBuilderX nativePrototype;
        if (signature->at(0).first != VT_VOID) {
            uint32_t returnType = asmjitTypeFromMathVMType(signature->at(0).first);
            mainFunctionPrototype.setRet(returnType);
            nativePrototype.setRet(returnType);
        }
        compiler.addFunc(kFuncConvHost, mainFunctionPrototype);

        // create native function
        X86GpVar nativeFunction(compiler);
        compiler.mov(nativeFunction, imm_ptr(nativeFunctionAddress));

        // set input parameters
        vector<asmjit::Var> variables;
        for (uint16_t paramNumber = 1, scopeVarIndex = 0; paramNumber < signature->size(); ++paramNumber, ++scopeVarIndex) {
            VarType varType = signature->at(paramNumber).first;

            nativePrototype.setArg(scopeVarIndex, asmjitTypeFromMathVMType(varType));

            switch (varType) {
                case VT_DOUBLE: {
                    X86XmmVar a(compiler, asmjitTypeFromMathVMType(varType));
                    setXmmVariable(compiler, a, loadVariable(scopeVarIndex).getDoubleValue());
                    variables.push_back(a);
                    break;
                }
                case VT_STRING: {
                    X86GpVar a(compiler, asmjitTypeFromMathVMType(varType));
                    compiler.mov(a, (int64_t) loadVariable(scopeVarIndex).getStringValue());
                    variables.push_back(a);
                    break;
                }
                case VT_INT: {
                    X86GpVar a(compiler, asmjitTypeFromMathVMType(varType));
                    compiler.mov(a, loadVariable(scopeVarIndex).getIntValue());
                    variables.push_back(a);
                    break;
                }
                default:
                    throw InterpretationError(string("Wrong NativeFunction parameter type ") + typeToName(signature->at(0).first));
            }

            X86XmmVar input(compiler, asmjitTypeFromMathVMType(varType));
            setXmmVariable(compiler, input, loadVariable(scopeVarIndex).getDoubleValue());
            variables.push_back(input);
        }

        X86CallNode *callNativeFunction = compiler.call(nativeFunction, kFuncConvHost, nativePrototype);

        // bind arguments
        for (uint16_t i = 0; i < variables.size(); ++i) {
            callNativeFunction->setArg(i, variables[i]);
        }

        // set return variable
        switch (signature->at(0).first) {
            case VT_DOUBLE: {
                X86XmmVar retVariable(compiler, kX86VarTypeXmmSd);
                callNativeFunction->setRet(0, retVariable);
                compiler.ret(retVariable);
                break;
            }
            case VT_STRING:
            case VT_INT: {
                X86GpVar retVariable(compiler, kVarTypeInt64);
                callNativeFunction->setRet(0, retVariable);
                compiler.ret(retVariable);
                break;
            }
            case VT_VOID:
                break;
            default:
                throw InterpretationError(string("Wrong NativeFunction return type ") + typeToName(signature->at(0).first));
        }
        compiler.endFunc();

        // calling native function
        void *wrappedNativeFunction = compiler.make();
        switch (signature->at(0).first) {
            case VT_VOID:
                asmjit_cast<void (*)()>(wrappedNativeFunction)();
                break;
            case VT_DOUBLE:
                pushVariable(asmjit_cast<double (*)()>(wrappedNativeFunction)());
                break;
            case VT_INT:
                pushVariable(asmjit_cast<signedIntType(*)()>(wrappedNativeFunction)());
                break;
            case VT_STRING:
                pushVariable(asmjit_cast<char const *(*)()>(wrappedNativeFunction)());
                break;
            default:
                throw InterpretationError(string("Wrong NativeFunction return type ") + typeToName(signature->at(0).first));
        }

        runtime.release(wrappedNativeFunction);
    }

}