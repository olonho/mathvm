#include "machcode_generator.h"

#define XSAVE(reg) _.movq(tempXmmRegs[1], reg);
#define XLOAD(reg) _.movq(reg, tempXmmRegs[1]);
#define SAVED(reg, body) {XSAVE(reg) {body} XLOAD(reg) }
namespace mathvm {

    static void genADD1(X86Mem const &dest, int64_t operand, X86Assembler &_) {
        _.add(dest, operand);
    }


    static void genADD1(X86GpReg const &dest, int64_t operand, X86Assembler &_) {
        _.add(dest, operand);
    }


    static void genADD1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {
        _.mov(tempGpRegs[0], operand);
        _.add(dest, tempGpRegs[0]);
    }


    static void genADD1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_) {
        _.add(dest, operand);
    }


    static void genADD1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.add(dest, operand);
    }


    static void genADD1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.add(dest, operand);
    }


    static void genSUB1(X86Mem const &dest, int64_t operand, X86Assembler &_) {
        _.sub(dest, operand);
    }


    static void genSUB1(X86GpReg const &dest, int64_t operand, X86Assembler &_) {
        _.sub(dest, operand);
    }


    static void genSUB1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {
        _.mov(rax, operand);
        _.sub(dest, rax);
    }


    static void genSUB1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_) {
        _.sub(dest, operand);
    }


    static void genSUB1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.sub(dest, operand);
    }


    static void genSUB1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.sub(dest, operand);
    }


    static void genMUL1(X86Mem const &dest, int64_t operand, X86Assembler &_) {
        //could do better...
        if (operand == 2) {
            _.sal(dest, 1);
        }
        else if (operand == 4) {
            _.sal(dest,2);
        }
        else if (operand <= INT32_MAX && operand >= INT32_MIN) {
            _.imul(rax, dest, (int32_t) operand);
            _.mov(dest, rax);
        }
        else {
            _.push(rdx);
            _.mov(rdx, operand);
            _.mov(rax, dest);
            _.imul(rax, rdx);
            _.mov(dest, rax);
            _.pop(rdx);
        }
    }


    static void genMUL1(X86GpReg const &dest, int64_t operand, X86Assembler &_) {
        if (operand == 2) {
            _.sal(dest, 1);
        }
        else if (operand == 4) {
            _.sal(dest,2);
        }
        else if (operand <= INT32_MAX && operand >= INT32_MIN) {
            _.imul(rax, dest, (int32_t) operand);
            _.mov(dest, rax);
        }
        else {
            _.mov(rax, operand);
            _.imul(dest, rax);
        }
    }


    static void genMUL1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {
        _.mov(rax, dest);
        _.imul(rax,operand);
        _.mov(dest, rax);
    }


    static void genMUL1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_) {
        _.imul(dest,operand);
    }


    static void genMUL1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.mov(tempGpRegs[0], dest);
        _.imul(tempGpRegs[0], operand);
        _.mov(dest, tempGpRegs[0]);
    }


    static void genMUL1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.imul(dest, operand);
    }


    static void genDIV1(X86Mem const &dest, int64_t operand, X86Assembler &_) {
        _.push(rdx);
        _.mov(rax, dest);
        _.xor_(rdx, rdx);
        _.push(operand);
        _.idiv(ptr(rsp));
        _.pop(rdx);
        _.pop(rdx);
        _.mov(dest, rax);
    }


    static void genDIV1(X86GpReg const &dest, int64_t operand, X86Assembler &_) {
        _.mov(rax, dest);
        SAVED(rdx, {
            _.xor_(rdx, rdx);

            _.mov(dest, operand);
            _.idiv(dest);
            _.mov(dest, rax);
        })
    }


    static void genDIV1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {
        SAVED(rdx, {
            _.xor_(rdx, rdx);
            _.mov(rax, dest);
            _.idiv(operand);
        });
        _.mov(dest, rax);
    }


    static void genDIV1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_) {
        SAVED(rdx, {
        _.xor_(rdx, rdx);
        _.mov(rax, dest);
        _.idiv(operand);
        _.mov(dest, rax);
    });
    }


    static void genDIV1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.push(rdx);
        _.push(operand);
        _.xor_(rdx, rdx);
        _.mov(rax, dest);
        _.idiv(ptr(rsp));
        _.mov(dest, rax);
        _.pop(rdx);
        _.pop(rdx);
    }


    static void genDIV1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.push(rdx);
        _.push(operand);
        _.xor_(rdx, rdx);
        _.mov(rax, dest);
        _.idiv(ptr(rsp));
        _.mov(dest, rax);
        _.pop(rdx);
        _.pop(rdx);
    }


    static void genFADD1(X86Mem const &dest, double operand, X86Assembler &_) {
        _.movq(tempXmmRegs[0],dest);
        _.mov(rax, D2I(operand));
        _.movq(tempXmmRegs[1], rax);
        _.addsd(tempXmmRegs[0],tempXmmRegs[1]);
        _.movq(dest, tempXmmRegs[0]);
    }


    static void genFADD1(X86XmmReg const &dest, double operand, X86Assembler &_) {

    }


    static void genFADD1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genFADD1(X86XmmReg const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genFADD1(X86Mem const &dest, X86XmmReg const &operand, X86Assembler &_) {

    }


    static void genFADD1(X86XmmReg const &dest, X86XmmReg const &operand, X86Assembler &_) {

    }


    static void genFSUB1(X86Mem const &dest, double operand, X86Assembler &_) {

    }


    static void genFSUB1(X86XmmReg const &dest, double operand, X86Assembler &_) {

    }


    static void genFSUB1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genFSUB1(X86XmmReg const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genFSUB1(X86Mem const &dest, X86XmmReg const &operand, X86Assembler &_) {

    }


    static void genFSUB1(X86XmmReg const &dest, X86XmmReg const &operand, X86Assembler &_) {

    }


    static void genFMUL1(X86Mem const &dest, double operand, X86Assembler &_) {

    }


    static void genFMUL1(X86XmmReg const &dest, double operand, X86Assembler &_) {

    }


    static void genFMUL1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genFMUL1(X86XmmReg const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genFMUL1(X86Mem const &dest, X86XmmReg const &operand, X86Assembler &_) {

    }


    static void genFMUL1(X86XmmReg const &dest, X86XmmReg const &operand, X86Assembler &_) {

    }


    static void genFDIV1(X86Mem const &dest, double operand, X86Assembler &_) {

    }


    static void genFDIV1(X86XmmReg const &dest, double operand, X86Assembler &_) {

    }


    static void genFDIV1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genFDIV1(X86XmmReg const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genFDIV1(X86Mem const &dest, X86XmmReg const &operand, X86Assembler &_) {

    }


    static void genFDIV1(X86XmmReg const &dest, X86XmmReg const &operand, X86Assembler &_) {

    }


    static void genMOD1(X86Mem const &dest, int64_t operand, X86Assembler &_) {

    }


    static void genMOD1(X86GpReg const &dest, int64_t operand, X86Assembler &_) {

    }


    static void genMOD1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genMOD1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genMOD1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_) {

    }


    static void genMOD1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_) {

    }


    static void genLT1(X86Mem const &dest, int64_t operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.setl(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genLT1(X86GpReg const &dest, int64_t operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.setl(al);
        _.movzx(dest, al);
    }


    static void genLT1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {
        _.mov(rax, operand);
        _.cmp(dest, rax);
        _.setl(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genLT1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.setl(al);
        _.movzx(dest, al);
    }


    static void genLT1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.setl(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genLT1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.setl(al);
        _.movzx(dest, al);
    }


    static void genFLT1(X86Mem const &dest, double operand, X86Assembler &_) {

    }


    static void genFLT1(X86XmmReg const &dest, double operand, X86Assembler &_) {

    }


    static void genFLT1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genFLT1(X86XmmReg const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genFLT1(X86Mem const &dest, X86XmmReg const &operand, X86Assembler &_) {

    }


    static void genFLT1(X86XmmReg const &dest, X86XmmReg const &operand, X86Assembler &_) {

    }

    static void genLE1(X86Mem const &dest, int64_t operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.setle(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genLE1(X86GpReg const &dest, int64_t operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.setle(al);
        _.movzx(dest, al);
    }


    static void genLE1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {
        _.mov(rax, operand);
        _.cmp(dest, rax);
        _.setle(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genLE1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.setle(al);
        _.movzx(dest, al);
    }


    static void genLE1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.setle(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genLE1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.setle(al);
        _.movzx(dest, al);
    }


    static void genFLE1(X86Mem const &dest, double operand, X86Assembler &_) {

    }


    static void genFLE1(X86XmmReg const &dest, double operand, X86Assembler &_) {

    }


    static void genFLE1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genFLE1(X86XmmReg const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genFLE1(X86Mem const &dest, X86XmmReg const &operand, X86Assembler &_) {

    }


    static void genFLE1(X86XmmReg const &dest, X86XmmReg const &operand, X86Assembler &_) {

    }

    static void genEQ1(X86Mem const &dest, int64_t operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.sete(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genEQ1(X86GpReg const &dest, int64_t operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.sete(al);
        _.movzx(dest, al);
    }


    static void genEQ1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {
        _.mov(rax, operand);
        _.cmp(dest, rax);
        _.sete(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genEQ1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.sete(al);
        _.movzx(dest, al);
    }


    static void genEQ1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.sete(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genEQ1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.sete(al);
        _.movzx(dest, al);
    }

    static void genNEQ1(X86Mem const &dest, int64_t operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.setne(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genNEQ1(X86GpReg const &dest, int64_t operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.setne(al);
        _.movzx(dest, al);
    }


    static void genNEQ1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {
        _.mov(rax, operand);
        _.cmp(dest, rax);
        _.setne(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genNEQ1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.setne(al);
        _.movzx(dest, al);
    }


    static void genNEQ1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.setne(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genNEQ1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.cmp(dest, operand);
        _.setne(al);
        _.movzx(dest, al);
    }


    static void genOR1(X86Mem const &dest, int64_t operand, X86Assembler &_) {

    }


    static void genOR1(X86GpReg const &dest, int64_t operand, X86Assembler &_) {

    }


    static void genOR1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genOR1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genOR1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_) {

    }


    static void genOR1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_) {

    }


    static void genAND1(X86Mem const &dest, int64_t operand, X86Assembler &_) {

    }


    static void genAND1(X86GpReg const &dest, int64_t operand, X86Assembler &_) {

    }


    static void genAND1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genAND1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genAND1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_) {

    }


    static void genAND1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_) {

    }


    static void genLOR1(X86Mem const &dest, int64_t operand, X86Assembler &_) {

    }


    static void genLOR1(X86GpReg const &dest, int64_t operand, X86Assembler &_) {

    }


    static void genLOR1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genLOR1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genLOR1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_) {

    }


    static void genLOR1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_) {

    }


    static void genLAND1(X86Mem const &dest, int64_t operand, X86Assembler &_) {

    }


    static void genLAND1(X86GpReg const &dest, int64_t operand, X86Assembler &_) {

    }


    static void genLAND1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genLAND1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genLAND1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_) {

    }


    static void genLAND1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_) {

    }


    static void genXOR1(X86Mem const &dest, int64_t operand, X86Assembler &_) {

    }


    static void genXOR1(X86GpReg const &dest, int64_t operand, X86Assembler &_) {

    }


    static void genXOR1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genXOR1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genXOR1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_) {

    }


    static void genXOR1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_) {

    }

    static void genADD2(X86GpReg const &dest, int64_t fst, X86GpReg const &snd, X86Assembler &_) {
        _.add(dest, fst);
        _.add(dest, snd);
    }


    static void genADD2(X86GpReg const &dest, X86GpReg const &fst, int64_t snd, X86Assembler &_) {
        _.lea(dest, ptr(fst, snd));
    }


    static void genADD2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &snd, X86Assembler &_) {
        _.lea(dest,ptr(fst,snd));
    }


    static void genSUB2(X86GpReg const &dest, int64_t fst, X86GpReg const &snd, X86Assembler &_) {
        _.mov(dest, fst);
        _.sub(dest, snd);
    }


    static void genSUB2(X86GpReg const &dest, X86GpReg const &fst, int64_t snd, X86Assembler &_) {
        _.mov(dest, fst);
        _.sub(dest, snd);
    }


    static void genSUB2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &snd, X86Assembler &_) {
        _.mov(dest, fst);
        _.sub(dest, snd);
    }


    static void genMUL2(X86GpReg const &dest, int64_t fst, X86GpReg const &snd, X86Assembler &_) {
        //todo special cases
         _.mov(dest,fst);
        _.imul(dest, snd);
    }


    static void genMUL2(X86GpReg const &dest, X86GpReg const &fst, int64_t snd, X86Assembler &_) {
        genMUL2(dest, snd, fst, _);
    }


    static void genMUL2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &snd, X86Assembler &_) {
        _.mov(dest, snd);
        _.imul(dest, fst);
    }


    static void genDIV2(X86GpReg const &dest, int64_t fst, X86GpReg const &snd, X86Assembler &_) {
        if (dest == rdx) {
            if (snd == rdx) {
                SAVED(rcx,
                        _.mov(rcx, rdx);
                        _.mov(rax, fst);
                        _.cqo();
                        _.idiv(rcx);
                        _.mov(dest, rax);
                )
            }
            else {
                _.mov(rax, fst);
                _.cqo();
                _.idiv(snd);
                _.mov(rdx, rax);
            }
        } else {
            if (snd == rdx) {
                SAVED(rdx,
                        _.mov(rax, fst);
                        _.push(rdx);
                        _.cqo();
                        _.idiv(ptr(rsp));
                        _.mov(dest, rax);
                        _.pop(rax);
                )
            }
            else {
                SAVED(rdx,
                _.mov(rax, fst);
                _.cqo();
                _.idiv(snd);
                _.mov(dest, rax);
                )
            }
        }
    }


    static void genDIV2(X86GpReg const &dest, X86GpReg const &fst, int64_t  snd, X86Assembler &_) {
        //todo cases like 2
        if (snd == 2) {_.mov(dest, fst); _.sar(dest, 1); return; }

        if (dest == rdx) {
            if (fst == rdx) {
                SAVED(rcx,
                        _.mov(rax, fst);
                        _.cqo();
                        _.mov(rcx, snd);
                        _.idiv(rcx);
                        _.mov(dest, rax);
                );
            }
            else {
                        _.mov(rax, fst);
                        _.cqo();
                        _.push(snd);
                        _.idiv(ptr(rsp));
                        _.mov(dest, rax);
                        _.pop(rax);
            }
        } else {
            if (fst == rdx) {
                SAVED(rdx,
                        _.mov(rax, fst);
                        _.push(snd);
                        _.cqo();
                        _.idiv(ptr(rsp));
                        _.mov(dest, rax);
                        _.pop(rax);
                )
            }
            else {
                SAVED(rdx,
                        _.mov(rax, fst);
                        _.cqo();
                        _.push(snd);
                        _.idiv(ptr(rsp));
                        _.mov(dest, rax);
                        _.pop(rax);
                )
            }
        }

    }


    static void genDIV2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &snd, X86Assembler &_) {
        if (dest == rdx && fst == rdx && snd == rdx) {
            asmjit::Label error, end;
            _.test(rdx, rdx);
            _.jz(error);
            _.mov(rdx, 1);
            _.jmp(end);
            _.bind(error);
            _.mov(rdx, 0);
            _.mov(rax, 1);
            _.div(rdx);
            _.bind(end);
            _.mov(dest, 1);
            return;
        }
        if (dest == rdx && fst == rdx && snd != rdx) {
            _.mov(rax, rdx);
            _.cqo();
            _.idiv(snd);
            _.mov(dest, rax);
            return;
        }
        if (dest == rdx && fst != rdx && snd == rdx) {
            _.push(rdx);
            _.mov(fst, rax);
            _.cqo();
            _.idiv(ptr(rsp));
            _.mov(dest, rax);
            _.pop(rax);
            return;
        }
        if (dest == rdx && fst != rdx && snd != rdx) {
            _.mov(rax, fst);
            _.cqo();
            _.idiv(snd);
            _.mov(rdx, rax);
        }
        if (dest != rdx && fst == rdx && snd == rdx) {
            asmjit::Label error, end;
            _.test(rdx, rdx);
            _.jz(error);
            _.mov(rdx, 1);
            _.jmp(end);
            _.bind(error);
            _.mov(rdx, 0);
            _.mov(rax, 1);
            _.div(rdx);
            _.bind(end);
            _.mov(dest, 1);
            return;

        }
        if (dest != rdx && fst == rdx && snd != rdx) {
            SAVED(rdx,
                    _.mov(rax, rdx);
                    _.cqo();
                    _.idiv(snd);
                    _.mov(dest, rax);
            );
        }
        if (dest != rdx && fst != rdx && snd == rdx) {
            _.push(rdx);
            _.mov(rax, fst);
            _.cqo();
            _.idiv(ptr(rsp));
            _.mov(dest, rax);
            _.pop(rdx);
        }
        if (dest != rdx && fst != rdx && snd != rdx) {
            SAVED(rdx,
            _.mov(rax, fst);
            _.cqo();
            _.idiv(snd);
            _.mov(dest, rax);
            )
        }
    }


    static void genFADD2(X86Mem const &dest, double fst, X86Mem const &snd, X86Assembler &_) {


    }


    static void genFADD2(X86XmmReg const &dest, double fst, X86Mem const &snd, X86Assembler &_) {

    }


    static void genFADD2(X86Mem const &dest, double fst, X86XmmReg const &snd, X86Assembler &_) {

    }


    static void genFADD2(X86XmmReg const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFADD2(X86Mem const &dest, X86Mem const &fst, double    snd, X86Assembler &_) {

    }


    static void genFADD2(X86XmmReg const &dest, X86Mem const &fst, double    snd, X86Assembler &_) {

    }


    static void genFADD2(X86Mem const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFADD2(X86XmmReg const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFADD2(X86Mem const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_) {
    }


    static void genFADD2(X86XmmReg const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFADD2(X86Mem const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_) {

    }


    static void genFADD2(X86XmmReg const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_) {

    }


    static void genFADD2(X86Mem const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFADD2(X86XmmReg const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFADD2(X86Mem const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFADD2(X86XmmReg const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFSUB2(X86Mem const &dest, double fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFSUB2(X86XmmReg const &dest, double fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFSUB2(X86Mem const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFSUB2(X86XmmReg const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFSUB2(X86Mem const &dest, X86Mem const &fst, double    snd, X86Assembler &_) {

    }


    static void genFSUB2(X86XmmReg const &dest, X86Mem const &fst, double    snd, X86Assembler &_) {

    }


    static void genFSUB2(X86Mem const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFSUB2(X86XmmReg const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFSUB2(X86Mem const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFSUB2(X86XmmReg const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFSUB2(X86Mem const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_) {

    }


    static void genFSUB2(X86XmmReg const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_) {

    }


    static void genFSUB2(X86Mem const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFSUB2(X86XmmReg const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFSUB2(X86Mem const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFSUB2(X86XmmReg const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFMUL2(X86Mem const &dest, double fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFMUL2(X86XmmReg const &dest, double fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFMUL2(X86Mem const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFMUL2(X86XmmReg const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFMUL2(X86Mem const &dest, X86Mem const &fst, double    snd, X86Assembler &_) {

    }


    static void genFMUL2(X86XmmReg const &dest, X86Mem const &fst, double    snd, X86Assembler &_) {

    }


    static void genFMUL2(X86Mem const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFMUL2(X86XmmReg const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFMUL2(X86Mem const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFMUL2(X86XmmReg const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFMUL2(X86Mem const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_) {

    }


    static void genFMUL2(X86XmmReg const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_) {

    }


    static void genFMUL2(X86Mem const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_) {

    }

    static void genFMUL2(X86XmmReg const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_) {
    }


    static void genFMUL2(X86Mem const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFMUL2(X86XmmReg const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFDIV2(X86Mem const &dest, double fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFDIV2(X86XmmReg const &dest, double fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFDIV2(X86Mem const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFDIV2(X86XmmReg const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFDIV2(X86Mem const &dest, X86Mem const &fst, double    snd, X86Assembler &_) {
    }


    static void genFDIV2(X86XmmReg const &dest, X86Mem const &fst, double    snd, X86Assembler &_) {

    }


    static void genFDIV2(X86Mem const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFDIV2(X86XmmReg const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFDIV2(X86Mem const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFDIV2(X86XmmReg const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFDIV2(X86Mem const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_) {

    }


    static void genFDIV2(X86XmmReg const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_) {

    }


    static void genFDIV2(X86Mem const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFDIV2(X86XmmReg const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_) {

    }

    static void genFDIV2(X86Mem const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFDIV2(X86XmmReg const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genMOD2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_) {

    }


    static void genMOD2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_) {

    }


    static void genMOD2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_) {

    }


    static void genLT2(X86GpReg const &dest, int64_t fst, X86GpReg const &snd, X86Assembler &_) {
        _.cmp(snd, fst);
        _.setge(al);
        _.movzx(dest, al);
    }


    static void genLT2(X86GpReg const &dest, X86GpReg const &fst, int64_t snd, X86Assembler &_) {
        _.cmp(fst, snd);
        _.setle(al);
        _.movzx(dest, al);
    }


    static void genLT2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &snd, X86Assembler &_) {
        _.cmp(fst, snd);
        _.setle(al);
        _.movzx(dest, al);
    }

    static void genFLT2(X86Mem const &dest, double fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFLT2(X86XmmReg const &dest, double fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFLT2(X86Mem const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFLT2(X86XmmReg const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFLT2(X86Mem const &dest, X86Mem const &fst, double    snd, X86Assembler &_) {

    }


    static void genFLT2(X86XmmReg const &dest, X86Mem const &fst, double    snd, X86Assembler &_) {

    }


    static void genFLT2(X86Mem const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFLT2(X86XmmReg const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFLT2(X86Mem const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }

    static void genFLT2(X86XmmReg const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_) {
    }


    static void genFLT2(X86Mem const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_) {

    }


    static void genFLT2(X86XmmReg const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_) {

    }


    static void genFLT2(X86Mem const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFLT2(X86XmmReg const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFLT2(X86Mem const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFLT2(X86XmmReg const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genLE2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_) {
    }


    static void genLE2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_) {

    }


    static void genLE2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_) {

    }


    static void genFLE2(X86Mem const &dest, double fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFLE2(X86XmmReg const &dest, double fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFLE2(X86Mem const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFLE2(X86XmmReg const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFLE2(X86Mem const &dest, X86Mem const &fst, double    snd, X86Assembler &_) {

    }


    static void genFLE2(X86XmmReg const &dest, X86Mem const &fst, double    snd, X86Assembler &_) {

    }


    static void genFLE2(X86Mem const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFLE2(X86XmmReg const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFLE2(X86Mem const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFLE2(X86XmmReg const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFLE2(X86Mem const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_) {
    }


    static void genFLE2(X86XmmReg const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_) {

    }


    static void genFLE2(X86Mem const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFLE2(X86XmmReg const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_) {

    }


    static void genFLE2(X86Mem const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genFLE2(X86XmmReg const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_) {

    }


    static void genEQ2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_) {

    }


    static void genEQ2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_) {

    }


    static void genEQ2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_) {

    }


    static void genNEQ2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_) {

    }


    static void genNEQ2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_) {

    }


    static void genNEQ2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_) {

    }


    static void genOR2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_) {

    }


    static void genOR2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_) {

    }


    static void genOR2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_) {

    }


    static void genAND2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_) {
    }


    static void genAND2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_) {

    }


    static void genAND2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_) {

    }


    static void genLOR2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_) {

    }


    static void genLOR2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_) {

    }


    static void genLOR2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_) {

    }


    static void genLAND2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_) {

    }


    static void genLAND2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_) {

    }


    static void genLAND2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_) {

    }


    static void genXOR2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_) {

    }


    static void genXOR2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_) {

    }


    static void genXOR2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_) {

    }

    static void genNEG1(X86Mem const &dest, int64_t operand, X86Assembler &_) {
        _.mov(dest, -operand);
    }


    static void genNEG1(X86GpReg const &dest, int64_t operand, X86Assembler &_) {
        _.mov(dest, -operand);
    }


    static void genNEG1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {
        _.mov(rax, operand);
        _.neg(rax);
        _.mov(dest, rax);
    }


    static void genNEG1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_) {
        _.mov(dest, operand);
        _.neg(dest);
    }


    static void genNEG1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.mov(rax, operand);
        _.neg(rax);
        _.mov(dest, rax);
    }


    static void genNEG1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.mov(dest, operand);
        _.neg(dest);
    }


    static void genNOT1(X86Mem const &dest, int64_t operand, X86Assembler &_) {
        if (operand) _.mov(dest, 0);
        else _.mov(dest,1);
    }


    static void genNOT1(X86GpReg const &dest, int64_t operand, X86Assembler &_) {
        if (operand) _.mov(dest, 0);
        else _.mov(dest,1);
    }


    static void genNOT1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {
        _.mov(rax, operand);
        _.test(rax, rax);
        _.setz(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genNOT1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_) {
        _.mov(rax, operand);
        _.test(rax, rax);
        _.setz(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genNOT1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.mov(rax, operand);
        _.test(rax, rax);
        _.setz(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genNOT1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_) {
        _.test(operand, operand);
        _.setz(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genFNEG1(X86Mem const &dest, double operand, X86Assembler &_) {

    }


    static void genFNEG1(X86XmmReg const &dest, double operand, X86Assembler &_) {

    }


    static void genFNEG1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genFNEG1(X86XmmReg const &dest, X86Mem const &operand, X86Assembler &_) {

    }


    static void genFNEG1(X86Mem const &dest, X86XmmReg const &operand, X86Assembler &_) {

    }


    static void genFNEG1(X86XmmReg const &dest, X86XmmReg const &operand, X86Assembler &_) {

    }


    void MachCodeGenerator::genBinOp(IR::BinOp::Type type, RegOrMem lhs, IR::Atom const *const fst, IR::Atom const *const snd, X86Assembler &_) {
        if (type == IR::BinOp::BO_XOR && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genXOR2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_XOR && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genXOR2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_XOR && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genXOR2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genLAND2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genLAND2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genLAND2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genLOR2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genLOR2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genLOR2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genAND2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genAND2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genAND2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genOR2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genOR2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genOR2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genNEQ2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genNEQ2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genNEQ2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genEQ2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genEQ2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genEQ2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLE2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLE2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLE2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLE2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFLE2(lhs.xmm, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFLE2(lhs.mem, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLE2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLE2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLE2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLE2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFLE2(lhs.xmm, locate(fst->asVariable()).mem, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFLE2(lhs.mem, locate(fst->asVariable()).mem, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLE2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLE2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLE2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLE2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genLE2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genLE2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genLE2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLT2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLT2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLT2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLT2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFLT2(lhs.xmm, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFLT2(lhs.mem, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLT2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLT2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLT2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLT2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFLT2(lhs.xmm, locate(fst->asVariable()).mem, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFLT2(lhs.mem, locate(fst->asVariable()).mem, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLT2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLT2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLT2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLT2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genLT2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genLT2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genLT2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genMOD2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genMOD2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genMOD2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFDIV2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFDIV2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFDIV2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFDIV2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFDIV2(lhs.xmm, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFDIV2(lhs.mem, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFDIV2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFDIV2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFDIV2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFDIV2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFDIV2(lhs.xmm, locate(fst->asVariable()).mem, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFDIV2(lhs.mem, locate(fst->asVariable()).mem, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFDIV2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFDIV2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFDIV2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFDIV2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFMUL2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFMUL2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFMUL2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFMUL2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFMUL2(lhs.xmm, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFMUL2(lhs.mem, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFMUL2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFMUL2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFMUL2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFMUL2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFMUL2(lhs.xmm, locate(fst->asVariable()).mem, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFMUL2(lhs.mem, locate(fst->asVariable()).mem, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFMUL2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFMUL2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFMUL2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFMUL2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFSUB2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFSUB2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFSUB2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFSUB2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFSUB2(lhs.xmm, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFSUB2(lhs.mem, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFSUB2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFSUB2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFSUB2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFSUB2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFSUB2(lhs.xmm, locate(fst->asVariable()).mem, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFSUB2(lhs.mem, locate(fst->asVariable()).mem, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFSUB2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFSUB2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFSUB2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFSUB2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFADD2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFADD2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFADD2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFADD2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFADD2(lhs.xmm, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFADD2(lhs.mem, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFADD2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFADD2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFADD2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFADD2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFADD2(lhs.xmm, locate(fst->asVariable()).mem, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFADD2(lhs.mem, locate(fst->asVariable()).mem, contents(snd->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFADD2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFADD2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFADD2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFADD2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genDIV2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genDIV2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genDIV2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genMUL2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genMUL2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genMUL2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genSUB2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genSUB2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genSUB2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genADD2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genADD2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genADD2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _);
            return;
        }
        throw std::invalid_argument("Bad argument for reduced binary operation generator");
    }


    void MachCodeGenerator::genReducedBinOp(IR::BinOp::Type type, RegOrMem lhs, IR::Atom const *const operand, X86Assembler &_) {
        if (type == IR::BinOp::BO_XOR && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genXOR1(lhs.gp, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_XOR && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genXOR1(lhs.mem, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_XOR && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genXOR1(lhs.gp, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_XOR && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genXOR1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_XOR && lhs.isGp() && operand->isInt()) {
            genXOR1(lhs.gp, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_XOR && lhs.isMem() && operand->isInt()) {
            genXOR1(lhs.mem, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genLAND1(lhs.gp, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genLAND1(lhs.mem, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genLAND1(lhs.gp, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genLAND1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isGp() && operand->isInt()) {
            genLAND1(lhs.gp, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isMem() && operand->isInt()) {
            genLAND1(lhs.mem, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genLOR1(lhs.gp, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genLOR1(lhs.mem, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genLOR1(lhs.gp, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genLOR1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isGp() && operand->isInt()) {
            genLOR1(lhs.gp, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isMem() && operand->isInt()) {
            genLOR1(lhs.mem, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genAND1(lhs.gp, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genAND1(lhs.mem, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genAND1(lhs.gp, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genAND1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isGp() && operand->isInt()) {
            genAND1(lhs.gp, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isMem() && operand->isInt()) {
            genAND1(lhs.mem, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genOR1(lhs.gp, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genOR1(lhs.mem, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genOR1(lhs.gp, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genOR1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isGp() && operand->isInt()) {
            genOR1(lhs.gp, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isMem() && operand->isInt()) {
            genOR1(lhs.mem, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genNEQ1(lhs.gp, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genNEQ1(lhs.mem, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genNEQ1(lhs.gp, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genNEQ1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isGp() && operand->isInt()) {
            genNEQ1(lhs.gp, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isMem() && operand->isInt()) {
            genNEQ1(lhs.mem, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genEQ1(lhs.gp, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genEQ1(lhs.mem, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genEQ1(lhs.gp, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genEQ1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isGp() && operand->isInt()) {
            genEQ1(lhs.gp, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isMem() && operand->isInt()) {
            genEQ1(lhs.mem, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFLE1(lhs.xmm, locate(operand->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFLE1(lhs.mem, locate(operand->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFLE1(lhs.xmm, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFLE1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isXmm() && operand->isDouble()) {
            genFLE1(lhs.xmm, contents(operand->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && operand->isDouble()) {
            genFLE1(lhs.mem, contents(operand->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genLE1(lhs.gp, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genLE1(lhs.mem, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genLE1(lhs.gp, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genLE1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isGp() && operand->isInt()) {
            genLE1(lhs.gp, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isMem() && operand->isInt()) {
            genLE1(lhs.mem, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFLT1(lhs.xmm, locate(operand->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFLT1(lhs.mem, locate(operand->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFLT1(lhs.xmm, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFLT1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isXmm() && operand->isDouble()) {
            genFLT1(lhs.xmm, contents(operand->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && operand->isDouble()) {
            genFLT1(lhs.mem, contents(operand->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genLT1(lhs.gp, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genLT1(lhs.mem, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genLT1(lhs.gp, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genLT1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isGp() && operand->isInt()) {
            genLT1(lhs.gp, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isMem() && operand->isInt()) {
            genLT1(lhs.mem, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genMOD1(lhs.gp, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genMOD1(lhs.mem, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genMOD1(lhs.gp, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genMOD1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isGp() && operand->isInt()) {
            genMOD1(lhs.gp, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isMem() && operand->isInt()) {
            genMOD1(lhs.mem, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFDIV1(lhs.xmm, locate(operand->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFDIV1(lhs.mem, locate(operand->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFDIV1(lhs.xmm, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFDIV1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && operand->isDouble()) {
            genFDIV1(lhs.xmm, contents(operand->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && operand->isDouble()) {
            genFDIV1(lhs.mem, contents(operand->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFMUL1(lhs.xmm, locate(operand->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFMUL1(lhs.mem, locate(operand->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFMUL1(lhs.xmm, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFMUL1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && operand->isDouble()) {
            genFMUL1(lhs.xmm, contents(operand->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && operand->isDouble()) {
            genFMUL1(lhs.mem, contents(operand->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFSUB1(lhs.xmm, locate(operand->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFSUB1(lhs.mem, locate(operand->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFSUB1(lhs.xmm, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFSUB1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && operand->isDouble()) {
            genFSUB1(lhs.xmm, contents(operand->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && operand->isDouble()) {
            genFSUB1(lhs.mem, contents(operand->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFADD1(lhs.xmm, locate(operand->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFADD1(lhs.mem, locate(operand->asVariable()).xmm, _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFADD1(lhs.xmm, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFADD1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && operand->isDouble()) {
            genFADD1(lhs.xmm, contents(operand->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && operand->isDouble()) {
            genFADD1(lhs.mem, contents(operand->asDouble()), _);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genDIV1(lhs.gp, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genDIV1(lhs.mem, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genDIV1(lhs.gp, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genDIV1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isGp() && operand->isInt()) {
            genDIV1(lhs.gp, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isMem() && operand->isInt()) {
            genDIV1(lhs.mem, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genMUL1(lhs.gp, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genMUL1(lhs.mem, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genMUL1(lhs.gp, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genMUL1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isGp() && operand->isInt()) {
            genMUL1(lhs.gp, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isMem() && operand->isInt()) {
            genMUL1(lhs.mem, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genSUB1(lhs.gp, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genSUB1(lhs.mem, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genSUB1(lhs.gp, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genSUB1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isGp() && operand->isInt()) {
            genSUB1(lhs.gp, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isMem() && operand->isInt()) {
            genSUB1(lhs.mem, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genADD1(lhs.gp, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genADD1(lhs.mem, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genADD1(lhs.gp, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genADD1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isGp() && operand->isInt()) {
            genADD1(lhs.gp, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isMem() && operand->isInt()) {
            genADD1(lhs.mem, contents(operand->asInt()), _);
            return;
        }
        throw std::invalid_argument("Bad argument for reduced binary operation generator");
    }


    void MachCodeGenerator::genUnOp(IR::UnOp::Type type, RegOrMem lhs, IR::Atom const *const operand, X86Assembler &_) {
        if (type == IR::UnOp::UO_FNEG && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFNEG1(lhs.xmm, locate(operand->asVariable()).xmm, _);
            return;
        }
        if (type == IR::UnOp::UO_FNEG && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFNEG1(lhs.mem, locate(operand->asVariable()).xmm, _);
            return;
        }
        if (type == IR::UnOp::UO_FNEG && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFNEG1(lhs.xmm, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::UnOp::UO_FNEG && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFNEG1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::UnOp::UO_FNEG && lhs.isXmm() && operand->isDouble()) {
            genFNEG1(lhs.xmm, contents(operand->asDouble()), _);
            return;
        }
        if (type == IR::UnOp::UO_FNEG && lhs.isMem() && operand->isDouble()) {
            genFNEG1(lhs.mem, contents(operand->asDouble()), _);
            return;
        }
        if (type == IR::UnOp::UO_NOT && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genNOT1(lhs.gp, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::UnOp::UO_NOT && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genNOT1(lhs.mem, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::UnOp::UO_NOT && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genNOT1(lhs.gp, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::UnOp::UO_NOT && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genNOT1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::UnOp::UO_NOT && lhs.isGp() && operand->isInt()) {
            genNOT1(lhs.gp, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::UnOp::UO_NOT && lhs.isMem() && operand->isInt()) {
            genNOT1(lhs.mem, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::UnOp::UO_NEG && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genNEG1(lhs.gp, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::UnOp::UO_NEG && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genNEG1(lhs.mem, locate(operand->asVariable()).gp, _);
            return;
        }
        if (type == IR::UnOp::UO_NEG && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genNEG1(lhs.gp, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::UnOp::UO_NEG && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genNEG1(lhs.mem, locate(operand->asVariable()).mem, _);
            return;
        }
        if (type == IR::UnOp::UO_NEG && lhs.isGp() && operand->isInt()) {
            genNEG1(lhs.gp, contents(operand->asInt()), _);
            return;
        }
        if (type == IR::UnOp::UO_NEG && lhs.isMem() && operand->isInt()) {
            genNEG1(lhs.mem, contents(operand->asInt()), _);
            return;
        }
        throw std::invalid_argument("Bad argument for reduced binary operation generator");
    }

}