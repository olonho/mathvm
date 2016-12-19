#pragma once


#include <cstdint>
#include <mathvm.h>
#include <asmjit/asmjit.h>
#include "bytecode_translator.h"

namespace mathvm {

uint32_t mapType(VarType type) {
    using namespace asmjit;
    switch (type) {
        case VT_DOUBLE:
            return kVarTypeFp64;
        case VT_INT:
            return kVarTypeInt64;
        case VT_STRING:
            return kVarTypeIntPtr;
        default:
            assert(false);
    }
}

asmjit::FuncBuilderX build_prototype(Signature const &signature) {
    using namespace asmjit;
    auto result = FuncBuilderX(kCallConvHost);

    if (signature[0].first != VT_VOID) {
        result.setRet(mapType(signature[0].first));
    }

    for (size_t i = 1; i < signature.size(); ++i) {
        result.addArg(mapType(signature[i].first));
    }

    return result;
}

void* wrap_native_call(Signature const &signature, void const *native_ptr) {
    using namespace asmjit;
    using namespace details;
    using namespace x86;
    static JitRuntime runtime;
    X86Assembler assembler(&runtime);
    X86Compiler compiler(&assembler);

    compiler.addFunc(FuncBuilder1 < void * , void const * > (kCallConvHost));
    X86GpVar args = compiler.newIntPtr();
    compiler.setArg(0, args);

    FuncBuilderX prototype = build_prototype(signature);
    vector <X86Var> vars;
    vars.reserve(signature.size() - 1);

    for (uint32_t i = 0; i < signature.size() - 1; ++i) {
        switch (signature[i + 1].first) {
            case VT_DOUBLE: {
                X86XmmVar var = compiler.newXmmSd();
                compiler.movq(var, qword_ptr(args, i * sizeof(double)));
                vars.push_back(var);
                break;
            }
            case VT_INT: {
                X86GpVar var = compiler.newGpVar(prototype.getArg(i));
                compiler.mov(var, qword_ptr(args, i * sizeof(int64_t)));
                vars.push_back(var);
                break;
            }
            case VT_STRING: {
                X86GpVar var = compiler.newInt64();
                compiler.mov(var, qword_ptr(args, i * sizeof(int64_t)));
                vars.push_back(var);
                break;
            }
            default:
                assert(false);
        }
    }

    X86CallNode *call = compiler.call(imm_ptr(native_ptr), prototype);
    for (uint32_t i = 0; i < vars.size(); ++i) {
        call->setArg(i, vars[i]);
    }

    if (signature[0].first != VT_VOID) {
    }
    X86GpVar outerResult = compiler.newInt64();

    switch (signature[0].first) {
        case VT_DOUBLE: {
            X86XmmVar result = compiler.newXmm();
            call->setRet(0, result);
            compiler.movq(outerResult, result);
            break;
        }
        case VT_INT:
        case VT_STRING: {
            call->setRet(0, outerResult);
            break;
        }
        default:
            break;
    }

    compiler.ret(outerResult);
    compiler.endFunc();
    compiler.finalize();
    return assembler.make();
}

}