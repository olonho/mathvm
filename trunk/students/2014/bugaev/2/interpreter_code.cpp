#include "interpreter_code.hpp"
#include "bytecode_interpreter.hpp"

#include <dlfcn.h>

using namespace asmjit;
using namespace asmjit::x86;


namespace mathvm
{

Status *InterpreterCodeImpl::execute(vector<Var *> &)
{
    try {
        BytecodeInterpreter(this).interpret();
    } catch (BytecodeException const &e) {
        return Status::Error(/*"Error"*/e.what());
    }
    return Status::Ok();
}


uint16_t InterpreterCodeImpl::buildNativeFunction(NativeCallNode *node)
{
    void *rawCode = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
    if (!rawCode) {
        throw BytecodeGeneratorException("Native function is not found",
                                         node->position());
    }

    Signature const &signature = node->nativeSignature();
    assert(signature.size() >= 1);

    X86Assembler a(&m_runtime);

    X86GpReg gps[] = {rdi, rsi, rdx, rcx, r8, r9};
    size_t const gpCount = sizeof(gps) / sizeof(gps[0]);

    X86XmmReg xmms[] = {xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7};
    size_t const xmmCount = sizeof(xmms) / sizeof(xmms[0]);

    size_t intCount = 0;
    size_t doubleCount = 0;
    for (size_t i = 1; i < signature.size(); ++i) {
        intCount += signature[i].first != VT_DOUBLE;
        doubleCount += signature[i].first == VT_DOUBLE;
    }
    size_t stackShift = signature.size() - 1
            - std::min(gpCount, intCount) - std::min(xmmCount, doubleCount);
    if (!(stackShift & 1)) ++stackShift;
    stackShift *= sizeof(int64_t);

    a.sub(rsp, stackShift);
    a.mov(rax, rdi);

    size_t gpId = 0;
    size_t xmmId = 0;
    size_t stackId = 0;
    for(size_t i = 1; i < signature.size(); ++i) {
        size_t const shift = sizeof(int64_t) * (i - 1);

        if (signature[i].first != VT_DOUBLE && gpId < gpCount) {
            a.mov(gps[gpId++], ptr(rax, shift));
        } else if (signature[i].first == VT_DOUBLE && xmmId < xmmCount) {
            a.movq(xmms[xmmId++], ptr(rax, shift));
        } else {
            a.mov(r11, ptr(rax, shift)); 
            a.mov(ptr(rsp, sizeof(int64_t) * stackId++), r11);
        }
    }

    a.mov(rax, imm_ptr(rawCode));
    a.call(rax);

    if(signature[0].first == VT_DOUBLE)
        a.movq(rax, xmm0);

    a.add(rsp, stackShift);
    a.ret();

    void *code = a.make();
    if (!code) {
        throw BytecodeGeneratorException("Couldn't build native function",
                                         node->position());
    }

    return makeNativeFunction(node->nativeName(), signature, code);
}

}
