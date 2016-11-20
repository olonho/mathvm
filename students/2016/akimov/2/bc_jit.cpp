#include "bc_jit.h"

using namespace mathvm;
using namespace asmjit;

static size_t varSize(mathvm::VarType type) {
    switch (type) {
        case VT_INT:
            return sizeof(int64_t);
        case VT_DOUBLE:
            return sizeof(double);
        case VT_STRING:
            return sizeof(const char*);
        case VT_VOID:
            return 0;
    }
    return 0;
}

static void loadArgsFromMem(const Signature* signature, X86Compiler& c, X86GpVar& mem, vector<X86Var>& vars) {
    size_t offset = varSize(signature->at(0).first);
    for (size_t j = 0; j < signature->size() - 1; ++j) {
        switch (signature->at(j + 1).first) {
            case VT_INT: {
                X86GpVar v = c.newGpVar(TypeId<int64_t>::kId);
                c.mov(v, x86::qword_ptr(mem, offset));
                vars.push_back(v);
                offset += sizeof(int64_t);
                break;
            }
            case VT_STRING: {
                X86GpVar v = c.newGpVar(TypeId<const char*>::kId);
                c.mov(v, x86::qword_ptr(mem, offset));
                vars.push_back(v);
                offset += sizeof(const char*);
                break;
            }
            case VT_DOUBLE: {
                X86XmmVar v = c.newXmm();
                c.movq(v, x86::qword_ptr(mem, offset));
                vars.push_back(v);
                offset += sizeof(double);
                break;
            }
        }
    }
}

static FuncBuilderX builderFromSignature(const Signature* signature) {
    FuncBuilderX builder;
    switch (signature->at(0).first) {
        case VT_INT:
            builder.setRetT<int64_t>();
            break;
        case VT_DOUBLE:
            builder.setRetT<double>();
            break;
        case VT_STRING:
            builder.setRetT<const char*>();
            break;
        case VT_VOID:
            builder.setRetT<void>();
            break;
    }
    for (size_t j = 1; j < signature->size(); ++j) {
        switch (signature->at(j).first) {
            case VT_INT:
                builder.addArgT<int64_t>();
                break;
            case VT_DOUBLE:
                builder.addArgT<double>();
                break;
            case VT_STRING:
                builder.addArgT<const char*>();
                break;
        }
    }
    builder.setCallConv(kCallConvHost);
    return builder;
}

void* BytecodeJITHelper::compileNative(const Signature* signature, const void* ptr) {
    unique_ptr<JitRuntime> runtime(new JitRuntime());
    X86Assembler a(runtime.get());
    X86Compiler c(&a);
    _runtimes.push_back(std::move(runtime));
    //FileLogger logger(stderr);
    //a.setLogger(&logger);

    mathvm::VarType retType = signature->at(0).first;
    FuncBuilderX builder = builderFromSignature(signature);

    c.addFunc(FuncBuilder1<void, const char*>(kCallConvHost));
    X86GpVar mem = c.newGpVar(TypeId<const char*>::kId);
    c.setArg(0, mem);

    vector<X86Var> vars;
    loadArgsFromMem(signature, c, mem, vars);

    X86CallNode* call = c.call(imm_ptr(ptr), builder);
    for (size_t i = 0; i < vars.size(); ++i) {
        call->setArg(i, vars[i]);
    }

    switch (retType) {
        case VT_INT: {
            X86GpVar retVar = c.newGpVar(TypeId<int64_t>::kId);
            call->setRet(0, retVar);
            c.mov(x86::qword_ptr(mem), retVar);
            break;
        }
        case VT_STRING: {
            X86GpVar retVar = c.newGpVar(TypeId<const char*>::kId);
            call->setRet(0, retVar);
            c.mov(x86::qword_ptr(mem), retVar);
            break;
        }
        case VT_DOUBLE: {
            X86XmmVar v = c.newXmm();
            c.movq(x86::qword_ptr(mem), v);
            call->setRet(0, v);
            break;
        }
        case VT_VOID:
            break;
    }
    c.ret();
    c.endFunc();

    c.finalize();

    return a.make();
}
