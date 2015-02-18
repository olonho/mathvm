#include "mathvm_runtime.h"
#include "machcode_generator.h"
#include "../../../../../libs/asmjit/asmjit.h"

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