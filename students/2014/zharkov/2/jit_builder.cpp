#include "jit_builder.h"
#include <vector>
#include "asmjit/asmjit.h"
#include <exception>

namespace mathvm {
    using namespace asmjit;
    using namespace asmjit::x86;

    JitBuilder JitBuilder::instance_;

    void * JitBuilder::buildNativeProxy(Signature const &signature, const void *addr) {
        typedef long long word_t;
        vector<index_t> gp_args;
        gp_args.reserve(8);
        vector<index_t> double_args;
        double_args.reserve(6);
        vector<index_t> stack_args;
        stack_args.reserve(signature.size());

        for (index_t i = 1; i < signature.size(); ++i) {
            switch(signature[i].first) {
                case VT_DOUBLE:
                    if (double_args.size() < 8) {
                        double_args.push_back(i);
                    } else {
                        stack_args.push_back(i);
                    }

                    break;
                case VT_STRING:
                case VT_INT:
                    if (gp_args.size() < 6) {
                        gp_args.push_back(i);
                    } else {
                        stack_args.push_back(i);
                    }
                    break;
                default:
                    throw runtime_error("unexpected type in native");
            }
        }

        X86Assembler assembler(&runtime_);

        static X86GpReg gp_args_regs[6] = { rdi, rsi, rdx, rcx, r8, r9 };
        static X86XmmReg double_args_regs[8] = { xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7 };

        // rule of thumb: (used stack size + 8 % 16) == 0, i.e. (used stack slots & 1) != 0
        size_t stack_delta = (stack_args.size() + !(stack_args.size() & 1)) * sizeof(word_t);

        assembler.sub(rsp, imm(stack_delta));
        assembler.mov(r11, rdi); // first argument is a pointer to begin offset of arguments

        for (size_t i = 0; i < gp_args.size(); ++i) {
            assembler.mov(gp_args_regs[i], ptr(r11, gp_args[i] * sizeof(word_t)));
        }

        for (size_t i = 0; i < double_args.size(); ++i) {
            assembler.movsd(double_args_regs[i], ptr(r11, double_args[i] * sizeof(word_t)));
        }

        for (size_t i = 0; i < stack_args.size(); ++i) {
            assembler.mov(r10, ptr(r11, stack_args[i] * sizeof(word_t)));
            assembler.mov(ptr(rsp, i * sizeof(word_t)), r10);
        }

        assembler.mov(rax, imm(reinterpret_cast<word_t>(addr)));
        assembler.call(rax);

        assembler.add(rsp, stack_delta);
        assembler.ret();

        return assembler.make();
    }
}