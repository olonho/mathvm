#include "SimpleInterpreter.hpp"
#include "asmjit/asmjit.h"

using namespace asmjit;

namespace mathvm {
    static uint32_t asmjitTypeFromMathVMType(VarType type) {
        switch (type) {
            case VT_DOUBLE:
                return kX86VarTypeXmmSd;
            case VT_INT:
            case VT_STRING:
                return kVarTypeInt64;
            default:
                throw InterpretationError(string("Wrong NativeFunction type while converting to AsmJitType") + typeToName(type));
        }
    }

    void setXmmVariable(X86Compiler &c, XmmVar &v, double d) {
        X86GpVar temp = c.newGpVar();
        uint64_t *dd = (uint64_t *) (&d);
        c.mov(temp, *dd);
        c.movq(v, temp.m());
        c.unuse(temp);
    }

    void SimpleInterpreter::callNativeFunctionViaAsmJit(void *f, const Signature *signature, VarType returnType) {
        JitRuntime runtime;
        X86Compiler compiler(&runtime);

        FuncBuilderX mainFunctionPrototype;
        FuncBuilderX nativePrototype;
        if (returnType != VT_VOID) {
            uint32_t asmReturnType = asmjitTypeFromMathVMType(returnType);
            mainFunctionPrototype.setRet(asmReturnType);
            nativePrototype.setRet(asmReturnType);
        }
        compiler.addFunc(kFuncConvHost, mainFunctionPrototype);

        // create native function
        X86GpVar nativeFunction(compiler, kVarTypeIntPtr);
        compiler.mov(nativeFunction, imm_ptr(f));

        // set input parameters
        vector<asmjit::Var> variables;
        for (uint16_t paramNumber = 1, scopeVarIndex = 0; paramNumber < signature->size(); ++paramNumber, ++scopeVarIndex) {
            VarType varType = signature->at(paramNumber).first;

            nativePrototype.addArg(asmjitTypeFromMathVMType(varType));

            switch (varType) {
                case VT_DOUBLE: {
                    X86XmmVar a(compiler, asmjitTypeFromMathVMType(varType));
                    setXmmVariable(compiler, a, loadVariable(scopeVarIndex).getDoubleValue());
                    variables.push_back(a);
                    break;
                }
                case VT_STRING: {
                    X86GpVar a(compiler, asmjitTypeFromMathVMType(varType));
                    compiler.mov(a, (signedIntType) loadVariable(scopeVarIndex).getStringValue());
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
                    throw InterpretationError(string("Wrong NativeFunction parameter type ") + typeToName(returnType));
            }
        }

        X86CallNode *callNativeFunction = compiler.call(nativeFunction, kFuncConvHost, nativePrototype);

        // bind arguments
        for (uint32_t i = 0; i < variables.size(); ++i) {
            callNativeFunction->setArg(i, variables[i]);
        }

        // set return variable
        switch (returnType) {
            case VT_DOUBLE: {
                X86XmmVar retVariable(compiler, asmjitTypeFromMathVMType(returnType));
                callNativeFunction->setRet(0, retVariable);
                compiler.ret(retVariable);
                break;
            }
            case VT_STRING:
            case VT_INT: {
                X86GpVar retVariable(compiler, asmjitTypeFromMathVMType(returnType));
                callNativeFunction->setRet(0, retVariable);
                compiler.ret(retVariable);
                break;
            }
            case VT_VOID:
                compiler.ret();
                break;
            default:
                throw InterpretationError(string("Wrong NativeFunction return type ") + typeToName(returnType));
        }
        compiler.endFunc();

        // calling native function
        void *wrappedNativeFunction = compiler.make();
        switch (returnType) {
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
                throw InterpretationError(string("Wrong NativeFunction return type ") + typeToName(returnType));
        }

        runtime.release(wrappedNativeFunction);
    }
}