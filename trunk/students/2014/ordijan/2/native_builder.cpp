#include "native_builder.h"

namespace native {

using namespace mathvm;
using namespace asmjit;
using namespace asmjit::x86;

JitRuntime runtime;

/* static */
void* Builder::build(const Signature& signature, const void* address) {
    X86Assembler gasm(&runtime);

    static X86XmmReg xmmRegs[8] = {xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7};
    static X86GpReg  gpRegs[6] = {rdi, rsi, rdx, rcx, r8, r9};

    // Prolog
    gasm.push(rbp);
    gasm.mov(rbp, rsp);

    // Pass arguments
    if (signature.size() > 1) {
        uint8_t gpNumber = 0;
        uint8_t xmmNumber = 0;
        uint8_t offset = 0;
        std::vector<X86Mem> stack;

        gasm.mov(r11, rdi);

        for (uint32_t i = 1; i < signature.size(); i++, offset += 8) {
            switch (signature[i].first) {
            case VT_INT:
            case VT_STRING:
                if (gpNumber < 6)
                    gasm.mov(gpRegs[gpNumber++], ptr(r11, offset));
                else
                    stack.push_back(ptr(r11, offset));
                break;
            case VT_DOUBLE:
                if (xmmNumber < 8)
                    gasm.movsd(xmmRegs[xmmNumber++], ptr(r11, offset));
                else
                    stack.push_back(ptr(r11, offset));
                break;
            default:
                return NULL;
            }
        }

        while (!stack.empty()) {
            gasm.push(stack.back());
            stack.pop_back();
        }
    }

    // Call
    gasm.call(imm((int64_t)address));

    // Epilog
    gasm.mov(rsp, rbp);
    gasm.pop(rbp);
    gasm.ret();

    return gasm.make();
}

}
