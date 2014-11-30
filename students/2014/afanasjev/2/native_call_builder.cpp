#include "native_call_builder.h"

namespace mathvm {

asmjit::JitRuntime NativeCallBuilder::runtime;

nativeCall NativeCallBuilder::makeCall(NativeFunctionDescriptor const & descr) {
    using namespace asmjit;
    using namespace asmjit::x86;

    Signature const & sig = descr.signature();

    X86Assembler a(&runtime);

    X86GpReg gpRegs[maxGpArgs] = {rdi, rsi, rdx, rcx, r8, r9};
    X86XmmReg xmmRegs[maxXmmArgs] = {xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7};
    int curGp = 0, curXmm = 0, curStack = 0;
    
    int stackShift = calcNumberOfStackArgs(sig);
    if(stackShift % 2 == 0) {
        ++stackShift;
    }

    a.sub(rsp, 8 * stackShift);
    a.mov(rax, rdi);

    for(size_t i = 1; i < sig.size(); ++i) {
        int displ = 8 * (i - 1);

        if(sig[i].first != VT_DOUBLE && curGp < maxGpArgs) {
            a.mov(gpRegs[curGp], ptr(rax, displ));
            ++curGp;
            continue;
        } 

        if(sig[i].first == VT_DOUBLE && curXmm < maxXmmArgs) {
            a.movq(xmmRegs[curXmm], ptr(rax, displ));
            ++curXmm;
            continue;
        }

        // put on stack
        a.mov(r11, ptr(rax, displ)); 
        a.mov(ptr(rsp, 8 * curStack), r11);
        ++curStack;
    }

    a.mov(rax, imm_ptr((void*)descr.code()));
    a.call(rax);
    
    if(sig[0].first == VT_DOUBLE) {
        a.movq(rax, xmm0); 
    }

    a.add(rsp, 8 * stackShift);
    a.ret();
    
    nativeCall res = asmjit_cast<nativeCall>(a.make());
    return res;
}

int NativeCallBuilder::calcNumberOfStackArgs(Signature const & sig) {
    int gp = 0;
    int xmm = 0;

    for(size_t i = 1; i < sig.size(); ++i) {
        if(sig[i].first == VT_DOUBLE) {
            ++xmm;
        }
        else {
            ++gp;
        }
    }

    return std::max(0, gp - maxGpArgs) + std::max(0, xmm - maxXmmArgs);
}

}
