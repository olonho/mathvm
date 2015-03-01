#include "mathvm_runtime.h"

namespace mathvm {

    void mathvm::MvmRuntime::pushRegs(X86Assembler &_) {
                _.pushf();
                for (uint32_t i = 0; i < ALLOCABLE_REGS_COUNT; i++)
                    _.push(allocableRegs[i]);

                for (uint32_t i = 0; i < ALLOCABLE_REGS_COUNT; i++) {
                    _.movq(rax, xmm(i));
                    _.push(rax);
                }
            }

    const int64_t MvmRuntime::FNEG_MASK = 1LL<<63;// 0x8000000000000000;

    void mathvm::MvmRuntime::popRegs(X86Assembler &_)  {
        for (uint32_t i = 0; i < ALLOCABLE_REGS_COUNT; i++) {
            _.pop(rax);
            _.movq(xmm(ALLOCABLE_REGS_COUNT-1 - i), rax);
        }
        for (uint32_t i = 0; i < ALLOCABLE_REGS_COUNT; i++)
            _.pop(allocableRegs[ALLOCABLE_REGS_COUNT - 1 - i]);

        _.popf();
    }


}