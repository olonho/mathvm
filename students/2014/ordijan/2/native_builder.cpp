#include "native_builder.h"

namespace native {

using namespace mathvm;
using namespace asmjit;
using namespace asmjit::x86;

JitRuntime runtime;

uint8_t stackArgs(const Signature& signature) {
    uint8_t xmm = 0, gp = 0;
    for (uint32_t i = 1; i < signature.size(); i++)
        if (signature[i].first == VT_DOUBLE) ++xmm;
        else ++gp;
    return std::max(0, xmm - 8) + std::max(gp - 6, 0);
}

/* static */
void* Builder::build(const Signature& signature, const void* address) {
    X86Assembler _(&runtime);

    static X86XmmReg xmmRegs[8] = {xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7};
    static X86GpReg  gpRegs[6] = {rdi, rsi, rdx, rcx, r8, r9};

    uint8_t onStack = stackArgs(signature);

    // Prolog
    if (!(onStack & 1)) onStack++; // align stack to 16 byte
    _.sub(rsp, onStack * 8);

    // Pass arguments
    if (signature.size() > 1) {
        uint8_t gpNumber = 0;
        uint8_t xmmNumber = 0;
        uint16_t offset = 0;
        uint8_t stackArg = 0;

        _.mov(r11, rdi);

        for (uint32_t i = 1; i < signature.size(); i++, offset += 8)
            switch (signature[i].first) {
            case VT_INT:
            case VT_STRING:
                if (gpNumber < 6)
                    _.mov(gpRegs[gpNumber++], ptr(r11, offset));
                else {
                    _.mov(r12, ptr(r11, offset));
                    _.mov(ptr(rsp, 8 * stackArg++), r12);
                }
                break;
            case VT_DOUBLE:
                if (xmmNumber < 8)
                    _.movsd(xmmRegs[xmmNumber++], ptr(r11, offset));
                else  {
                    _.mov(r12, ptr(r11, offset));
                    _.mov(ptr(rsp, 8 * stackArg++), r12);
                }
                break;
            default:
                return NULL;
            }
    }

    // Call
    _.mov(rax, imm((int64_t)address));
    _.call(rax);

    // Epilog
    _.add(rsp, onStack * 8);
    _.ret();

    return _.make();
}

}
