#include "machcode_generator.h"
#include "../ir/ir.h"
#include "../exceptions.h"
#include "mathvm_runtime.h"

#define XSAVE(reg) _.movq(tempXmmRegs[1], reg);
#define XLOAD(reg) _.movq(reg, tempXmmRegs[1]);
#define SAVED(reg, body) {XSAVE(reg) {body} XLOAD(reg) }
namespace mathvm {

    static void genADD1(X86Mem const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        if (!operand) return;
        if (operand == 1) { _.inc(dest); }
        else _.add(dest, operand);
    }


    static void genADD1(X86GpReg const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        if (!operand) return;
        if (operand == 1) { _.inc(dest); }
        else _.add(dest, operand);
    }


    static void genADD1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(tempGpRegs[0], operand);
        _.add(dest, tempGpRegs[0]);
    }


    static void genADD1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.add(dest, operand);
    }


    static void genADD1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.add(dest, operand);
    }


    static void genADD1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.add(dest, operand);
    }


    static void genSUB1(X86Mem const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.sub(dest, operand);
    }


    static void genSUB1(X86GpReg const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        if (!operand) return;
        if (operand == 1) { _.dec(dest); }
        else _.add(dest, operand);
    }


    static void genSUB1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, operand);
        _.sub(dest, rax);
    }


    static void genSUB1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.sub(dest, operand);
    }


    static void genSUB1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.sub(dest, operand);
    }


    static void genSUB1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.sub(dest, operand);
    }


    static void genMUL1(X86Mem const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
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


    static void genMUL1(X86GpReg const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
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


    static void genMUL1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, dest);
        _.imul(rax,operand);
        _.mov(dest, rax);
    }


    static void genMUL1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.imul(dest,operand);
    }


    static void genMUL1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(tempGpRegs[0], dest);
        _.imul(tempGpRegs[0], operand);
        _.mov(dest, tempGpRegs[0]);
    }


    static void genMUL1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.imul(dest, operand);
    }


    static void genDIV1(X86Mem const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.push(rdx);
        _.mov(rax, dest);
        _.cqo();
        _.push(operand);
        _.idiv(qword_ptr(rsp));
        _.pop(rdx);
        _.pop(rdx);
        _.mov(dest, rax);
    }


    static void genDIV1(X86GpReg const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, dest);
        SAVED(rdx, {
            _.cqo();
            _.mov(dest, operand);
            _.idiv(dest);
            _.mov(dest, rax);
        })
    }


    static void genDIV1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        SAVED(rdx, {
            _.mov(rax, dest);
            _.cqo();
            _.idiv(operand);
        });
        _.mov(dest, rax);
    }


    static void genDIV1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        SAVED(rdx, {
        _.mov(rax, dest);
            _.cqo();
        _.idiv(operand);
        _.mov(dest, rax);
    });
    }


    static void genDIV1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.push(rdx);
        _.push(operand);
        _.mov(rax, dest);
        _.cqo();
        _.idiv(qword_ptr(rsp));
        _.mov(dest, rax);
        _.pop(rdx);
        _.pop(rdx);
    }


    static void genDIV1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.push(rdx);
        _.push(operand);
        _.mov(rax, dest);
        _.cqo();
        _.idiv(qword_ptr(rsp));
        _.mov(dest, rax);
        _.pop(rdx);
        _.pop(rdx);
    }


    static void genFADD1(X86Mem const &dest, double operand, X86Assembler &_, MvmRuntime& runtime) {

        _.movq(tempXmmRegs[1], ptr_abs(Ptr(runtime.vivify(operand))));
        _.addsd(tempXmmRegs[1], dest);
        _.movq(dest, tempXmmRegs[1]);
    }


    static void genFADD1(X86XmmReg const &dest, double operand, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(tempXmmRegs[0], ptr_abs(Ptr(runtime.vivify(operand))));
        _.addsd(dest, tempXmmRegs[0]);
    }


    static void genFADD1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(tempXmmRegs[0], dest);
        _.addsd(tempXmmRegs[0], operand);
        _.movq(dest, tempXmmRegs[0]);
    }


    static void genFADD1(X86XmmReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.addsd(dest, operand);
    }


    static void genFADD1(X86Mem const &dest, X86XmmReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(tempXmmRegs[0], operand);
        _.addsd(tempXmmRegs[0], dest);
        _.movq(dest,tempXmmRegs[0]);
    }


    static void genFADD1(X86XmmReg const &dest, X86XmmReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.addsd(tempXmmRegs[0], operand);
    }


    static void genFSUB1(X86Mem const &dest, double operand, X86Assembler &_, MvmRuntime& runtime) {
       _.movq(tempXmmRegs[0], dest);
        _.mov(tempGpRegs[0], ptr_abs(Ptr(runtime.vivify(operand))));

       _.movq(tempXmmRegs[1], tempGpRegs[0]);
       _.subsd(tempXmmRegs[0], tempXmmRegs[1]);
       _.movq(dest, tempXmmRegs[0]); 
    }


    static void genFSUB1(X86XmmReg const &dest, double operand, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(tempXmmRegs[0], ptr_abs(Ptr(runtime.vivify(operand))));
        _.subsd(dest, tempXmmRegs[0]);
    }


    static void genFSUB1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(tempXmmRegs[0], dest);
        _.subsd(tempXmmRegs[0], operand);
        _.movq(dest, tempXmmRegs[0]);
    }


    static void genFSUB1(X86XmmReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.subsd(dest, operand);
    }


    static void genFSUB1(X86Mem const &dest, X86XmmReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
	_.movq(tempXmmRegs[0], dest);
	_.subsd(tempXmmRegs[0], operand);
	_.movq(dest, tempXmmRegs[0]);
    }


    static void genFSUB1(X86XmmReg const &dest, X86XmmReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
	_.subsd(dest, operand);
    }


    static void genFMUL1(X86Mem const &dest, double operand, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(tempXmmRegs[0], ptr_abs(Ptr(runtime.vivify(operand))));
        _.mulsd(tempXmmRegs[0], dest);
        _.movq(dest, tempXmmRegs[0]);
    }


    static void genFMUL1(X86XmmReg const &dest, double operand, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(tempXmmRegs[0], ptr_abs(Ptr(runtime.vivify(operand))));
        _.mulsd(dest, tempXmmRegs[0]);
    }


    static void genFMUL1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(tempXmmRegs[0], operand);
        _.mulsd(tempXmmRegs[0], dest);
        _.movq(dest, tempXmmRegs[0]);
    }


    static void genFMUL1(X86XmmReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mulsd(dest, operand);
    }


    static void genFMUL1(X86Mem const &dest, X86XmmReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(tempXmmRegs[0], operand);
        _.mulsd(tempXmmRegs[0], dest);
        _.movq(dest, tempXmmRegs[0]);
    }


    static void genFMUL1(X86XmmReg const &dest, X86XmmReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mulsd(dest, operand);
    }


    static void genFDIV1(X86Mem const &dest, double operand, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(tempXmmRegs[1], ptr_abs(Ptr(runtime.vivify(operand))));
        _.movq(tempXmmRegs[0], dest);
        _.divsd(tempXmmRegs[0], tempXmmRegs[1]);
        _.movq(dest, tempXmmRegs[0]);
    }


    static void genFDIV1(X86XmmReg const &dest, double operand, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(tempXmmRegs[1], ptr_abs(Ptr(runtime.vivify(operand))));
        _.divsd(dest, tempXmmRegs[1]);
    }


    static void genFDIV1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {

        _.movq(tempXmmRegs[0], dest);
        _.divsd(tempXmmRegs[0], operand);
        _.movq(dest, tempXmmRegs[0]);
    }


    static void genFDIV1(X86XmmReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
       _.divsd(dest, operand);
    }


    static void genFDIV1(X86Mem const &dest, X86XmmReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(tempXmmRegs[0], dest);
        _.divsd(tempXmmRegs[0], operand);
        _.movq(dest, tempXmmRegs[0]);
    }


    static void genFDIV1(X86XmmReg const &dest, X86XmmReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.divsd(dest, operand);
    }


    static void genMOD1(X86Mem const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, dest);
        SAVED(rdx, {
            _.mov(rax, dest);
            _.cqo();
            _.mov(dest, operand);
            _.idiv(dest);
            _.mov(dest, rdx);
        })
    }


    static void genMOD1(X86GpReg const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        if (dest == rdx) {
            _.mov(rax, dest);
            _.push(operand);
            _.idiv(ptr(rsp));
            _.add(rsp, 8);
        }
        else {
            _.mov(rax, dest);
            SAVED(rdx, {
                _.cqo();
                _.mov(dest, operand);
                _.idiv(dest);
                _.mov(dest, rdx);
            })
        }
    }


    static void genMOD1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, dest);
        SAVED(rdx, {
            _.mov(rax, dest);
            _.cqo();
            _.idiv(operand);
            _.mov(dest, rdx);
        })
    }


    static void genMOD1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, dest);
        if (dest == rdx) {
            _.cqo();
            _.idiv(operand);
        }
        else {
            SAVED(rdx,
                    {
                        _.cqo();
                        _.idiv(operand);
                        _.mov(dest, rdx);
                    });
        }
    }


    static void genMOD1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        SAVED(rdx, {
            _.mov(rax, dest);
            _.cqo();
            _.idiv(operand);
            _.mov(dest, rdx);
        })
    }


    static void genMOD1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
            if (dest == rdx && operand != rdx) {
_.mov(rax, rdx);
_.cqo();
_.idiv(operand);
return;
}
            if (dest != rdx && operand != rdx) {
SAVED(rdx, {
    _.mov(rax, dest);
    _.cqo();
    _.idiv(operand);
    _.mov(dest, rdx);
});
return; }
            if (dest == rdx && operand == rdx) {
_.mov(rax, rdx);
_.push(rax);
_.cqo();
_.idiv(ptr(rsp));
_.pop(rax);
_.mov(dest, rdx);

return; }
            if (dest != rdx && operand == rdx) {
SAVED(rdx,
        {
            _.mov(rax, dest);
            _.mov(dest, rdx);
            _.cqo();
            _.idiv(dest);
            _.mov(dest, rdx);
        }
)
return;
}
    }


    static void genLT1(X86Mem const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.setl(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genLT1(X86GpReg const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.setl(al);
        _.movzx(dest, al);
    }


    static void genLT1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, operand);
        _.cmp(dest, rax);
        _.setl(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genLT1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.setl(al);
        _.movzx(dest, al);
    }


    static void genLT1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.setl(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genLT1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.setl(al);
        _.movzx(dest, al);
    }

    static void genLE1(X86Mem const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.setle(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genLE1(X86GpReg const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.setle(al);
        _.movzx(dest, al);
    }


    static void genLE1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, operand);
        _.cmp(dest, rax);
        _.setle(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genLE1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.setle(al);
        _.movzx(dest, al);
    }


    static void genLE1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.setle(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genLE1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.setle(al);
        _.movzx(dest, al);
    }



    static void genEQ1(X86Mem const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.sete(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genEQ1(X86GpReg const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.sete(al);
        _.movzx(dest, al);
    }


    static void genEQ1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, operand);
        _.cmp(dest, rax);
        _.sete(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genEQ1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.sete(al);
        _.movzx(dest, al);
    }


    static void genEQ1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.sete(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genEQ1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.sete(al);
        _.movzx(dest, al);
    }

    static void genNEQ1(X86Mem const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.setne(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genNEQ1(X86GpReg const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.setne(al);
        _.movzx(dest, al);
    }


    static void genNEQ1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, operand);
        _.cmp(dest, rax);
        _.setne(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genNEQ1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.setne(al);
        _.movzx(dest, al);
    }


    static void genNEQ1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.setne(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genNEQ1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(dest, operand);
        _.setne(al);
        _.movzx(dest, al);
    }


    static void genOR1(X86Mem const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.or_(dest, operand);
    }


    static void genOR1(X86GpReg const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.or_(dest, operand);
    }


    static void genOR1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, operand);
        _.or_(dest, rax);
    }


    static void genOR1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.or_(dest, operand);
    }


    static void genOR1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.or_(dest, operand);
    }


    static void genOR1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.or_(dest, operand);
    }


    static void genAND1(X86Mem const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.and_(dest, operand);
    }


    static void genAND1(X86GpReg const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.and_(dest, operand);
    }


    static void genAND1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, operand);
        _.and_(dest, rax);
    }


    static void genAND1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.and_(dest, operand);
    }


    static void genAND1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.and_(dest, operand);
    }


    static void genAND1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.and_(dest, operand);
    }


    static void genLOR1(X86Mem const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        if (operand) _.mov(dest, 1);
    }


    static void genLOR1(X86GpReg const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        if (operand) _.mov(dest, 1);
    }


    static void genLOR1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, operand);
        _.or_(dest, rax);
    }


    static void genLOR1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.or_(dest, operand);
    }


    static void genLOR1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.or_(dest, operand);
    }


    static void genLOR1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.or_(dest, operand);
    }


    static void genLAND1(X86Mem const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        if (!operand) _.mov(dest, 0);
    }


    static void genLAND1(X86GpReg const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        if(!operand) _.xor_(dest, dest);
    }


    static void genLAND1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        asmjit::Label end =  _.newLabel();
         _.mov(rax, dest);
        _.test(rax, rax);
        _.jz(end);
        _.mov(rax, operand);
        _.test(rax, rax);
        _.jz(end);
        _.mov(dest, 1);
        _.bind(end);
    }


    static void genLAND1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        asmjit::Label end =  _.newLabel();

        _.test(dest, dest);
        _.jz(end);
        _.mov(rax, operand);
        _.test(rax, rax);
        _.jz(end);
        _.mov(dest, 1);
        _.bind(end);
    }


    static void genLAND1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        asmjit::Label end =  _.newLabel();
        _.mov(rax, dest);
        _.test(rax, rax);
        _.jz(end);
        _.test(operand, operand);
        _.jz(end);
        _.mov(dest, 1);
        _.bind(end);
    }


    static void genLAND1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        asmjit::Label end =  _.newLabel();
        _.test(dest, dest);
        _.jz(end);
        _.test(operand, operand);
        _.jz(end);
        _.mov(dest, 1);
        _.bind(end);
    }


    static void genXOR1(X86Mem const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.xor_(dest, operand);
    }


    static void genXOR1(X86GpReg const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.xor_(dest, operand);
    }


    static void genXOR1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, operand);
        _.xor_(dest, rax);
    }


    static void genXOR1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.xor_(dest, operand);
    }


    static void genXOR1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.xor_(dest, operand);
    }


    static void genXOR1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.xor_(dest, operand);
    }

    static void genADD2(X86GpReg const &dest, int64_t fst, X86GpReg const &snd, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, fst);
        _.add(dest, snd);
    }


    static void genADD2(X86GpReg const &dest, X86GpReg const &fst, int64_t snd, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, fst);
        _.add(dest, snd);
    }


    static void genADD2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &snd, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, fst);
        _.add(dest, snd);
    }


    static void genSUB2(X86GpReg const &dest, int64_t fst, X86GpReg const &snd, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, fst);
        _.sub(dest, snd);
    }


    static void genSUB2(X86GpReg const &dest, X86GpReg const &fst, int64_t snd, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, fst);
        _.sub(dest, snd);
    }


    static void genSUB2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &snd, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, fst);
        _.sub(dest, snd);
    }


    static void genMUL2(X86GpReg const &dest, int64_t fst, X86GpReg const &snd, X86Assembler &_, MvmRuntime& runtime) {
        //todo special cases
         _.mov(dest,fst);
        _.imul(dest, snd);
    }


    static void genMUL2(X86GpReg const &dest, X86GpReg const &fst, int64_t snd, X86Assembler &_, MvmRuntime& runtime) {
        genMUL2(dest, snd, fst, _, runtime);
    }


    static void genMUL2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &snd, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, snd);
        _.imul(dest, fst);
    }


    static void genDIV2(X86GpReg const &dest, int64_t fst, X86GpReg const &snd, X86Assembler &_, MvmRuntime& runtime) {
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
                        _.idiv(qword_ptr(rsp));
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


    static void genDIV2(X86GpReg const &dest, X86GpReg const &fst, int64_t  snd, X86Assembler &_, MvmRuntime& runtime) {
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
                        _.idiv(qword_ptr(rsp));
                        _.mov(dest, rax);
                        _.pop(rax);
            }
        } else {
            if (fst == rdx) {
                SAVED(rdx,
                        _.mov(rax, fst);
                        _.push(snd);
                        _.cqo();
                        _.idiv(qword_ptr(rsp));
                        _.mov(dest, rax);
                        _.pop(rax);
                )
            }
            else {
                SAVED(rdx,
                        _.mov(rax, fst);
                        _.cqo();
                        _.push(snd);
                        _.idiv(qword_ptr(rsp));
                        _.mov(dest, rax);
                        _.pop(rax);
                )
            }
        }

    }


    static void genDIV2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &snd, X86Assembler &_, MvmRuntime& runtime) {
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
            _.idiv(qword_ptr(rsp));
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
            _.idiv(qword_ptr(rsp));
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


    static void genFADD2(X86Mem const &dest, double fst, X86Mem const &snd, X86Assembler &_, MvmRuntime& runtime) {
	_.movq(tempXmmRegs[0], snd);
	_.addsd(tempXmmRegs[0], ptr_abs(Ptr(runtime.vivify(fst))));
	_.movq(dest, tempXmmRegs[0]);
    }


    static void genFADD2(X86XmmReg const &dest, double fst, X86Mem const &snd, X86Assembler &_, MvmRuntime& runtime) {
	_.movq(dest, snd);
	_.addsd(dest, ptr_abs(Ptr(runtime.vivify(fst))));
    }


    static void genFADD2(X86Mem const &dest, double fst, X86XmmReg const &snd, X86Assembler &_, MvmRuntime& runtime) {
	_.movq(tempXmmRegs[0], ptr_abs(Ptr(runtime.vivify(fst))));
	_.addsd(tempXmmRegs[0], snd); 
	_.movq(dest, tempXmmRegs[0]);
    }

    static void genFADD2(X86XmmReg const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
	_.movq(dest, snd);
	_.movq(tempXmmRegs[0], ptr_abs(Ptr(runtime.vivify(fst))));
	_.addsd(dest, tempXmmRegs[0]);
    }


    static void genFADD2(X86Mem const &dest, X86Mem const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
	_.movq(tempXmmRegs[0], ptr_abs(Ptr(runtime.vivify(snd))));
	_.addsd(tempXmmRegs[0], fst);
	_.movq(dest, tempXmmRegs[0]);
    }


    static void genFADD2(X86XmmReg const &dest, X86Mem const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
	_.movq(dest, fst);
	_.addsd(dest, ptr_abs(Ptr(runtime.vivify(snd))));
    }


    static void genFADD2(X86Mem const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
	_.movq(tempXmmRegs[0], fst);
	_.addsd(tempXmmRegs[0], snd);
	_.movq(dest, tempXmmRegs[0]);
    }


    static void genFADD2(X86XmmReg const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
	_.movq(dest, fst);
	_.addsd(dest, snd);
    }


    static void genFADD2(X86Mem const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
	_.movq(tempXmmRegs[0], fst);
	_.addsd(tempXmmRegs[0], snd);
	_.movq(dest, tempXmmRegs[0]);
    }


    static void genFADD2(X86XmmReg const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
	_.movq(dest, fst);
	_.addsd(dest, snd);
    }


    static void genFADD2(X86Mem const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
	_.movq(tempXmmRegs[0], fst);
	_.addsd(tempXmmRegs[0], ptr_abs(Ptr(runtime.vivify(snd))));
	_.movq(dest, tempXmmRegs[0]);
    }


    static void genFADD2(X86XmmReg const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
	_.movq(dest, fst);
	_.addsd(dest,ptr_abs(Ptr(runtime.vivify(snd))));
    }


    static void genFADD2(X86Mem const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
	_.movq(tempXmmRegs[0], fst);
	_.addsd(tempXmmRegs[0], snd);
	_.movq(dest, tempXmmRegs[0]);
    }


    static void genFADD2(X86XmmReg const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
	_.movq(dest,fst);
	_.addsd(dest, snd);
    }


    static void genFADD2(X86Mem const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
	_.movq(tempXmmRegs[0], fst);
	_.addsd(tempXmmRegs[0], snd);
	_.movq(dest, tempXmmRegs[0]);
    }


    static void genFADD2(X86XmmReg const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
	_.movq(dest, fst);
	_.addsd(dest, snd);
    }


    static void genFSUB2(X86Mem const &dest, double fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
         _.movq(xmm0,  ptr_abs(Ptr(runtime.vivify(fst))));
        _.subsd(xmm0, snd);
        _.movq(dest, xmm0);
    }


    static void genFSUB2(X86XmmReg const &dest, double fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(dest,  ptr_abs(Ptr(runtime.vivify(fst))));
        _.subsd(dest, snd);
    }


    static void genFSUB2(X86Mem const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0,  ptr_abs(Ptr(runtime.vivify(fst))));
        _.subsd(xmm0, snd);
        _.movq(dest, xmm0);
    }


    static void genFSUB2(X86XmmReg const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(dest,  ptr_abs(Ptr(runtime.vivify(fst))));
        _.subsd(dest, snd);
    }


    static void genFSUB2(X86Mem const &dest, X86Mem const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, fst);
        _.movq(xmm1,ptr_abs(Ptr(runtime.vivify(snd))));
        _.subsd(xmm0, xmm1);
        _.movq(dest, xmm0);
    }


    static void genFSUB2(X86XmmReg const &dest, X86Mem const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(dest, fst);
        _.movq(xmm0, ptr_abs(Ptr(runtime.vivify(snd))));
        _.subsd(dest, xmm0);
    }


    static void genFSUB2(X86Mem const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, fst);
        _.subsd(xmm0, snd);
        _.movq(dest, xmm0); 
    }


    static void genFSUB2(X86XmmReg const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(dest, fst);
        _.subsd(dest, snd);
    }


    static void genFSUB2(X86Mem const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, fst);
        _.subsd(xmm0, snd);
        _.movq(dest, xmm0);
    }


    static void genFSUB2(X86XmmReg const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(dest, fst);
        _.subsd(dest, snd);
    }


    static void genFSUB2(X86Mem const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
_.movq(xmm0, fst);
        _.movq(xmm1, ptr_abs(Ptr(runtime.vivify(snd))));
        _.subsd(xmm0, xmm1);
        _.movq(dest, xmm0);
    }


    static void genFSUB2(X86XmmReg const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, ptr_abs(Ptr(runtime.vivify(snd))));
        _.movq(dest, fst);
        _.subsd(dest, xmm0);
    }


    static void genFSUB2(X86Mem const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0,fst);
        _.subsd(xmm0, snd);
        _.movq(dest, xmm0);
    }


    static void genFSUB2(X86XmmReg const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(dest, fst);
        _.subsd(dest, snd);
    }


    static void genFSUB2(X86Mem const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, fst);
        _.subsd(xmm0, snd);
        _.movq(dest, xmm0);
    }


    static void genFSUB2(X86XmmReg const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(dest, fst);
        _.mulsd(dest, snd);
    }


    static void genFMUL2(X86Mem const &dest, double fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {

        _.movq(xmm0,ptr_abs(Ptr(runtime.vivify(fst))));
        _.mulsd(xmm0, snd);
        _.movq(dest, xmm0);
    }


    static void genFMUL2(X86XmmReg const &dest, double fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {

        _.movq(xmm0,ptr_abs(Ptr(runtime.vivify(fst))));
        _.mulsd(xmm0, snd);
        _.movq(dest, xmm0);
    }


    static void genFMUL2(X86Mem const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0,ptr_abs(Ptr(runtime.vivify(fst))));
        _.mulsd(xmm0, snd);
        _.movq(dest, xmm0);
    }


    static void genFMUL2(X86XmmReg const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(dest, snd);
        _.mulsd(dest, ptr_abs(Ptr(runtime.vivify(fst))));
    }


    static void genFMUL2(X86Mem const &dest, X86Mem const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0,ptr_abs(Ptr(runtime.vivify(snd))));
        _.mulsd(xmm0, fst);
        _.movq(dest, xmm0);
    }


    static void genFMUL2(X86XmmReg const &dest, X86Mem const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {

        _.movq(dest,ptr_abs(Ptr(runtime.vivify(snd))));
        _.mulsd(dest, fst);
    }


    static void genFMUL2(X86Mem const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {

        _.movq(xmm0, fst);
        _.mulsd(xmm0, snd);
        _.movq(dest, xmm0);
    }


    static void genFMUL2(X86XmmReg const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(dest, fst);
        _.mulsd(dest, snd);
    }


    static void genFMUL2(X86Mem const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, fst);
        _.mulsd(xmm0, snd);
        _.movq(dest, xmm0);
    }


    static void genFMUL2(X86XmmReg const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
_.movq(dest, snd);
        _.mulsd(dest, fst);
    }


    static void genFMUL2(X86Mem const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {

        _.movq(xmm0,ptr_abs(Ptr(runtime.vivify(snd))));
        _.mulsd(xmm0, fst);
        _.movq(dest, xmm0);
    }


    static void genFMUL2(X86XmmReg const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {

        _.movq(dest,ptr_abs(Ptr(runtime.vivify(snd))));
        _.mulsd(dest, fst);
    }


    static void genFMUL2(X86Mem const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, snd);
        _.mulsd(xmm0, fst);
        _.movq(dest, xmm0);
    }

    static void genFMUL2(X86XmmReg const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(dest, fst);
        _.mulsd(dest, snd);
    }


    static void genFMUL2(X86Mem const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
    _.movq(xmm0, fst);
        _.mulsd(xmm0, snd);
        _.movq(dest, xmm0);
    }


    static void genFMUL2(X86XmmReg const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
_.movq(dest, fst);
        _.mulsd(dest, snd);
    }





    static void genFDIV2(X86Mem const &dest, double fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {

        _.movq(xmm0,ptr_abs(Ptr(runtime.vivify(fst))));
        _.divsd(xmm0, snd);
        _.movq(dest, xmm0);
    }


    static void genFDIV2(X86XmmReg const &dest, double fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {

        _.movq(xmm0,ptr_abs(Ptr(runtime.vivify(fst))));
        _.divsd(xmm0, snd);
        _.movq(dest, xmm0);
    }


    static void genFDIV2(X86Mem const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {

        _.movq(xmm0,ptr_abs(Ptr(runtime.vivify(fst))));
        _.divsd(xmm0, snd);
        _.movq(dest, xmm0);
    }


    static void genFDIV2(X86XmmReg const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {

        _.movq(xmm0,ptr_abs(Ptr(runtime.vivify(fst))));
        _.movq(dest, snd);
        _.divsd(dest, xmm0);
    }


    static void genFDIV2(X86Mem const &dest, X86Mem const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {

        _.movq(xmm0,ptr_abs(Ptr(runtime.vivify(snd))));
        _.divsd(xmm0, fst);
        _.movq(dest, xmm0);
    }


    static void genFDIV2(X86XmmReg const &dest, X86Mem const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {

        _.movq(xmm0,ptr_abs(Ptr(runtime.vivify(snd))));
        _.movq(dest, xmm0);
        _.divsd(dest, fst);
    }


    static void genFDIV2(X86Mem const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, fst);
        _.divsd(xmm0, snd);
        _.movq(dest, xmm0);
    }


    static void genFDIV2(X86XmmReg const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(dest, fst);
        _.divsd(dest, snd);
    }


    static void genFDIV2(X86Mem const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, fst);
        _.divsd(xmm0, snd);
        _.movq(dest, xmm0);
    }


    static void genFDIV2(X86XmmReg const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(dest, snd);
        _.divsd(dest, fst);
    }


    static void genFDIV2(X86Mem const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, fst);
        _.movq(xmm1,ptr_abs(Ptr(runtime.vivify(snd))));
        _.divsd(xmm0, xmm1);
        _.movq(dest, xmm0);
    }


    static void genFDIV2(X86XmmReg const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(dest, fst);
        _.divsd(dest, ptr_abs(Ptr(runtime.vivify(snd))));
    }


    static void genFDIV2(X86Mem const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, fst);
        _.divsd(xmm0, snd);
        _.movq(dest, xmm0);
    }

    static void genFDIV2(X86XmmReg const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {

        _.movq(dest, fst);
        _.divsd(dest, snd);
    }


    static void genFDIV2(X86Mem const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, fst);
        _.divsd(xmm0, snd);
        _.movq(dest, xmm0);
    }


    static void genFDIV2(X86XmmReg const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(dest, fst);
        _.divsd(dest, snd);
    }



    static void genMOD2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        if (dest == rdx && snd == rdx) {
            _.mov(rax, fst);
            _.push(snd);
            _.cqo();
            _.idiv(qword_ptr(rsp));
            _.add(rsp, 8);
            return;
        }
        if (dest != rdx && snd == rdx) {
            _.mov(rax, fst);
            _.push(rdx);
            _.cqo();
            _.idiv(qword_ptr(rsp));
            _.mov(dest, rdx);
            _.pop(rdx);
            return;
        }
        if (dest != rdx && snd != rdx) {
            _.mov(rax, fst);
            _.push(rdx);
            _.cqo();
            _.idiv(snd);
            _.mov(dest, rdx);
            _.pop(rdx);
            return;
        }
    }


    static void genMOD2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.push(snd);
        if (dest == rdx) {
            _.mov(rax, fst);
            _.cqo();
            _.idiv(ptr(rsp));
            _.add(rsp, 8);
        }
        else {
            SAVED(rdx, {
                _.mov(rax, fst);
                _.cqo();
                _.idiv(ptr(rsp));
                _.add(rsp,8);
                _.mov(dest, rdx);
            })
        }
    }


    static void genMOD2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {

        if (dest == rdx) {
            _.push(snd);
            _.mov(rax, fst);
            _.cqo();
            _.idiv(ptr(rsp));
            _.add(rsp,8);
        }
        else {
           SAVED(rdx, {
               _.push(snd);
            _.mov(rax, fst);
            _.cqo();
            _.idiv(ptr(rsp));
            _.add(rsp,8);
               _.mov(dest, rdx);
           }
           )
        }
    }


    static void genLT2(X86GpReg const &dest, int64_t fst, X86GpReg const &snd, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(snd, fst);
        _.setg(al);
        _.movzx(dest, al);
    }


    static void genLT2(X86GpReg const &dest, X86GpReg const &fst, int64_t snd, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(fst, snd);
        _.setl(al);
        _.movzx(dest, al);
    }


    static void genLT2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &snd, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(fst, snd);
        _.setl(al);
        _.movzx(dest, al);
    }

    static void genFLT2(X86Mem const &dest, double fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, ptr_abs(Ptr(runtime.vivify(fst))));
        _.ucomisd(xmm0, snd);
        _.setl(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genFLT2(X86GpReg const &dest, double fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, ptr_abs(Ptr(runtime.vivify(fst))));
        _.ucomisd(xmm0, snd);
        _.setl(al);
        _.movzx(dest, al);
    }


    static void genFLT2(X86Mem const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, ptr_abs(Ptr(runtime.vivify(fst))));
        _.ucomisd(xmm0, snd);
        _.setl(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genFLT2(X86GpReg const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, ptr_abs(Ptr(runtime.vivify(fst))));
        _.ucomisd(xmm0, snd);
        _.setl(al);
        _.movzx(dest, al);
    }


    static void genFLT2(X86Mem const &dest, X86Mem const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, ptr_abs(Ptr(runtime.vivify(snd))));
        _.ucomisd(xmm0, fst);
        _.setg(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genFLT2(X86GpReg const &dest, X86Mem const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, ptr_abs(Ptr(runtime.vivify(snd))));
        _.ucomisd(xmm0, fst);
        _.setg(al);
        _.movzx(dest, al);
    }


    static void genFLT2(X86Mem const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, fst);
        _.ucomisd(xmm0, snd);
        _.setl(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genFLT2(X86GpReg const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, fst);
        _.ucomisd(xmm0, snd);
        _.setl(al);
        _.movzx(dest, al);
    }


    static void genFLT2(X86Mem const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.ucomisd(snd, fst);
        _.setg(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }

    static void genFLT2(X86GpReg const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.ucomisd(snd, fst);
        _.setg(al);
        _.movzx(dest, al);
    }


    static void genFLT2(X86Mem const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.ucomisd(fst, ptr_abs(Ptr(runtime.vivify(snd))));
        _.setl(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genFLT2(X86GpReg const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.ucomisd(fst, ptr_abs(Ptr(runtime.vivify(snd))));
        _.setl(al);
        _.movzx(dest, al);
    }


    static void genFLT2(X86Mem const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.ucomisd(fst,snd);
        _.setl(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genFLT2(X86GpReg const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.ucomisd(fst,snd);
        _.setl(al);
        _.movzx(dest, al);
    }


    static void genFLT2(X86Mem const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.ucomisd(fst,snd);
        _.setl(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genFLT2(X86GpReg const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.ucomisd(fst,snd);
        _.setl(al);
        _.movzx(dest, al);
    }






    static void genFLE2(X86Mem const &dest, double fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, ptr_abs(Ptr(runtime.vivify(fst))));
        _.ucomisd(xmm0, snd);
        _.setle(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genFLE2(X86GpReg const &dest, double fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, ptr_abs(Ptr(runtime.vivify(fst))));
        _.ucomisd(xmm0, snd);
        _.setle(al);
        _.movzx(dest, al);
    }


    static void genFLE2(X86Mem const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, ptr_abs(Ptr(runtime.vivify(fst))));
        _.ucomisd(xmm0, snd);
        _.setle(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genFLE2(X86GpReg const &dest, double fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, ptr_abs(Ptr(runtime.vivify(fst))));
        _.ucomisd(xmm0, snd);
        _.setle(al);
        _.movzx(dest, al);
    }


    static void genFLE2(X86Mem const &dest, X86Mem const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, ptr_abs(Ptr(runtime.vivify(snd))));
        _.ucomisd(xmm0, fst);
        _.setge(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genFLE2(X86GpReg const &dest, X86Mem const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, ptr_abs(Ptr(runtime.vivify(snd))));
        _.ucomisd(xmm0, fst);
        _.setge(al);
        _.movzx(dest, al);
    }


    static void genFLE2(X86Mem const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, fst);
        _.ucomisd(xmm0, snd);
        _.setle(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genFLE2(X86GpReg const &dest, X86Mem const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, fst);
        _.ucomisd(xmm0, snd);
        _.setle(al);
        _.movzx(dest, al);
    }


    static void genFLE2(X86Mem const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.ucomisd(snd, fst);
        _.setge(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }

    static void genFLE2(X86GpReg const &dest, X86Mem const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.ucomisd(snd, fst);
        _.setge(al);
        _.movzx(dest, al);
    }


    static void genFLE2(X86Mem const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.ucomisd(fst, ptr_abs(Ptr(runtime.vivify(snd))));
        _.setle(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genFLE2(X86GpReg const &dest, X86XmmReg const &fst, double    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.ucomisd(fst, ptr_abs(Ptr(runtime.vivify(snd))));
        _.setle(al);
        _.movzx(dest, al);
    }


    static void genFLE2(X86Mem const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.ucomisd(fst,snd);
        _.setle(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genFLE2(X86GpReg const &dest, X86XmmReg const &fst, X86Mem const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.ucomisd(fst,snd);
        _.setle(al);
        _.movzx(dest, al);
    }


    static void genFLE2(X86Mem const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.ucomisd(fst,snd);
        _.setle(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genFLE2(X86GpReg const &dest, X86XmmReg const &fst, X86XmmReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.ucomisd(fst,snd);
        _.setle(al);
        _.movzx(dest, al);
    }
    
    
    
    
    
    

    static void genLE2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(snd, fst);
        _.setg(al);
        _.movzx(dest, al);
    }


    static void genLE2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(fst, snd);
        _.setle(al);
        _.movzx(dest, al);
    }


    static void genLE2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(fst, snd);
        _.setle(al);
        _.movzx(dest, al);
    }


    


    static void genEQ2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(snd, fst);
        _.sete(al);
        _.movzx(dest, al);
    }


    static void genEQ2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(fst, snd);
        _.sete(al);
        _.movzx(dest, al);
    }


    static void genEQ2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(snd, fst);
        _.sete(al);
        _.movzx(dest, al);
    }


    static void genNEQ2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(snd, fst);
        _.setne(al);
        _.movzx(dest, al);
    }


    static void genNEQ2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(fst, snd);
        _.setne(al);
        _.movzx(dest, al);
    }


    static void genNEQ2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.cmp(snd, fst);
        _.setne(al);
        _.movzx(dest, al);
    }


    static void genOR2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, snd);
        _.or_(dest, fst);
    }


    static void genOR2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_, MvmRuntime& runtime) {
_.mov(dest, fst);
        _.or_(dest, snd);
    }


    static void genOR2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, fst);
        _.or_(dest, snd);
    }


    static void genAND2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, fst);
        _.and_(dest, snd);
    }


    static void genAND2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, fst);
        _.and_(dest, snd);
    }


    static void genAND2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, fst);
        _.and_(dest, snd);
    }

// why nuh memory?
    static void genLOR2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, fst);
        _.or_(dest, snd);
        _.test(dest, dest);
        _.setnz(al);
        _.movzx(dest, al);
    }


    static void genLOR2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, fst);
        _.or_(dest, snd);
        _.test(dest, dest);
        _.setnz(al);
        _.movzx(dest, al);
    }


    static void genLOR2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, fst);
        _.or_(dest, snd);
        _.test(dest, dest);
        _.setnz(al);
        _.movzx(dest, al);
    }


    static void genLAND2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        if (fst) _.mov(dest, snd);
    }


    static void genLAND2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_, MvmRuntime& runtime) {
        if (snd) _.mov(dest, fst);
    }


    static void genLAND2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        asmjit::Label one = _.newLabel();
        asmjit::Label end = _.newLabel();
        _.test(fst, fst);
        _.jnz(one);
        _.xor_(dest, dest);
        _.jmp(end);

        _.bind(one);
        _.mov(dest, snd);
        _.bind(end);
    }


    static void genXOR2(X86GpReg const &dest, int64_t fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
_.mov(dest, fst);
        _.xor_(dest, snd);
    }


    static void genXOR2(X86GpReg const &dest, X86GpReg const &fst, int64_t    snd, X86Assembler &_, MvmRuntime& runtime) {
_.mov(dest, snd);
        _.xor_(dest, fst);
    }


    static void genXOR2(X86GpReg const &dest, X86GpReg const &fst, X86GpReg const &    snd, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, snd);
        _.xor_(dest, fst);
    }

    static void genNEG1(X86Mem const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, -operand);
    }


    static void genNEG1(X86GpReg const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, -operand);
    }


    static void genNEG1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, operand);
        _.neg(rax);
        _.mov(dest, rax);
    }


    static void genNEG1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, operand);
        _.neg(dest);
    }


    static void genNEG1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, operand);
        _.neg(rax);
        _.mov(dest, rax);
    }


    static void genNEG1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(dest, operand);
        _.neg(dest);
    }


    static void genNOT1(X86Mem const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        if (operand) _.mov(dest, 0);
        else _.mov(dest,1);
    }


    static void genNOT1(X86GpReg const &dest, int64_t operand, X86Assembler &_, MvmRuntime& runtime) {
        if (operand) _.mov(dest, 0);
        else _.mov(dest,1);
    }


    static void genNOT1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, operand);
        _.test(rax, rax);
        _.setz(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genNOT1(X86GpReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, operand);
        _.test(rax, rax);
        _.setz(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genNOT1(X86Mem const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, operand);
        _.test(rax, rax);
        _.setz(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genNOT1(X86GpReg const &dest, X86GpReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.test(operand, operand);
        _.setz(al);
        _.movzx(rax, al);
        _.mov(dest, rax);
    }


    static void genFNEG1(X86Mem const &dest, double operand, X86Assembler &_, MvmRuntime& runtime) {
        double d = -operand;
        _.mov(rax, D2I(d));

    }


    static void genFNEG1(X86XmmReg const &dest, double operand, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(dest, ptr_abs(Ptr(runtime.vivify(-operand))));
    }


    static void genFNEG1(X86Mem const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, ptr_abs(Ptr(&MvmRuntime::FNEG_MASK)));
        _.movq(xmm1, operand);
        _.xorps(xmm0,xmm1);
        _.movq(dest, xmm0);
    }


    static void genFNEG1(X86XmmReg const &dest, X86Mem const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm1, ptr_abs(Ptr(&MvmRuntime::FNEG_MASK)));
        _.movq(dest, operand);
        _.xorps(dest,xmm1);
    }


    static void genFNEG1(X86Mem const &dest, X86XmmReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(xmm0, ptr_abs(Ptr(&runtime.FNEG_MASK)));
        _.movq(xmm1, operand);
        _.xorps(xmm0,xmm1);
        _.movq(dest, xmm0);
    }


    static void genFNEG1(X86XmmReg const &dest, X86XmmReg const &operand, X86Assembler &_, MvmRuntime& runtime) {
        _.movq(dest, ptr_abs(Ptr(&MvmRuntime::FNEG_MASK)));
        _.movq(xmm1, operand);
        _.xorps(dest,xmm1);

    }

    static  void genCAST_I2D(X86Mem const& lhs, X86Mem const& operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, operand);
        _.cvtsi2sd(xmm0, rax);
        _.movq(lhs, xmm0);
    }

    static  void genCAST_I2D(X86Mem const& lhs, X86GpReg const& operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cvtsi2sd(xmm0, operand);
        _.movq(lhs, xmm0);
    }

    static  void genCAST_I2D(X86XmmReg const& lhs, X86Mem const& operand, X86Assembler &_, MvmRuntime& runtime) {
        _.mov(rax, operand);
        _.cvtsi2sd(lhs, rax);
    }

    static  void genCAST_I2D(X86XmmReg const& lhs, X86GpReg const& operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cvtsi2sd(lhs, operand);
    }


    static  void genCAST_D2I(X86Mem const& lhs, X86Mem const& operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cvttsd2si(rax, operand);
        _.mov(lhs, rax);
    }

    static  void genCAST_D2I(X86Mem const& lhs, X86XmmReg const& operand, X86Assembler &_, MvmRuntime& runtime) {
        _.cvttsd2si(rax, operand);
_.mov(lhs, rax);
    }

    static  void genCAST_D2I(X86GpReg const& lhs, X86Mem const& operand, X86Assembler &_, MvmRuntime& runtime) {
_.cvttsd2si(lhs, operand);
    }

    static  void genCAST_D2I(X86GpReg const& lhs, X86XmmReg const& operand, X86Assembler &_, MvmRuntime& runtime) {
_.cvttsd2si(lhs, operand);
    }

    void CodeGenerator::genBinOp(IR::BinOp::Type type, RegOrMem lhs, IR::Atom const *const fst, IR::Atom const *const snd, X86Assembler& _, MvmRuntime& runtime) {
        if (type == IR::BinOp::BO_XOR && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genXOR2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_XOR && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genXOR2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_XOR && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genXOR2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genLAND2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genLAND2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genLAND2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genLOR2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genLOR2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genLOR2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genAND2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genAND2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genAND2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genOR2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genOR2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genOR2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genNEQ2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genNEQ2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genNEQ2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genEQ2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genEQ2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genEQ2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLE2(lhs.gp, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLE2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLE2(lhs.gp, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLE2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFLE2(lhs.gp, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFLE2(lhs.mem, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLE2(lhs.gp, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLE2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLE2(lhs.gp, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLE2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFLE2(lhs.gp, locate(fst->asVariable()).mem, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFLE2(lhs.mem, locate(fst->asVariable()).mem, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isGp() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLE2(lhs.gp, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLE2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isGp() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLE2(lhs.gp, contents(fst->asDouble()), locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLE && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLE2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genLE2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genLE2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genLE2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLT2(lhs.gp, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLT2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLT2(lhs.gp, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLT2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFLT2(lhs.gp, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFLT2(lhs.mem, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLT2(lhs.gp, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLT2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLT2(lhs.gp, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLT2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFLT2(lhs.gp, locate(fst->asVariable()).mem, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFLT2(lhs.mem, locate(fst->asVariable()).mem, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isGp() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLT2(lhs.gp, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFLT2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isGp() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLT2(lhs.gp, contents(fst->asDouble()), locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FLT && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFLT2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genLT2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genLT2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genLT2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genMOD2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genMOD2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genMOD2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFDIV2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFDIV2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFDIV2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFDIV2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFDIV2(lhs.xmm, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFDIV2(lhs.mem, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFDIV2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFDIV2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFDIV2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFDIV2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFDIV2(lhs.xmm, locate(fst->asVariable()).mem, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFDIV2(lhs.mem, locate(fst->asVariable()).mem, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFDIV2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFDIV2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFDIV2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFDIV2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFMUL2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFMUL2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFMUL2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFMUL2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFMUL2(lhs.xmm, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFMUL2(lhs.mem, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFMUL2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFMUL2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFMUL2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFMUL2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFMUL2(lhs.xmm, locate(fst->asVariable()).mem, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFMUL2(lhs.mem, locate(fst->asVariable()).mem, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFMUL2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFMUL2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFMUL2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFMUL2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFSUB2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFSUB2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFSUB2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFSUB2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFSUB2(lhs.xmm, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFSUB2(lhs.mem, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFSUB2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFSUB2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFSUB2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFSUB2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFSUB2(lhs.xmm, locate(fst->asVariable()).mem, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFSUB2(lhs.mem, locate(fst->asVariable()).mem, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFSUB2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFSUB2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFSUB2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFSUB2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFADD2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFADD2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFADD2(lhs.xmm, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFADD2(lhs.mem, locate(fst->asVariable()).xmm, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFADD2(lhs.xmm, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isXmm() && snd->isDouble()) {
            genFADD2(lhs.mem, locate(fst->asVariable()).xmm, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFADD2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFADD2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFADD2(lhs.xmm, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFADD2(lhs.mem, locate(fst->asVariable()).mem, locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFADD2(lhs.xmm, locate(fst->asVariable()).mem, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && fst->isVariable() && locate(fst->asVariable()).isMem() && snd->isDouble()) {
            genFADD2(lhs.mem, locate(fst->asVariable()).mem, contents(snd->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFADD2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isXmm()) {
            genFADD2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFADD2(lhs.xmm, contents(fst->asDouble()), locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && fst->isDouble() && snd->isVariable() && locate(snd->asVariable()).isMem()) {
            genFADD2(lhs.mem, contents(fst->asDouble()), locate(snd->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genDIV2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genDIV2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genDIV2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genMUL2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genMUL2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genMUL2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genSUB2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genSUB2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genSUB2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genADD2(lhs.gp, locate(fst->asVariable()).gp, locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isGp() && fst->isVariable() && locate(fst->asVariable()).isGp() && snd->isInt()) {
            genADD2(lhs.gp, locate(fst->asVariable()).gp, contents(snd->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isGp() && fst->isInt() && snd->isVariable() && locate(snd->asVariable()).isGp()) {
            genADD2(lhs.gp, contents(fst->asInt()), locate(snd->asVariable()).gp, _, runtime);
            return;
        }
        throw  CodeGenerationError("Bad argument for reduced binary operation generator");
    }


    void  CodeGenerator::genReducedBinOp(IR::BinOp::Type type, RegOrMem lhs, IR::Atom const *const operand, X86Assembler &_, MvmRuntime& runtime) {
        if (type == IR::BinOp::BO_XOR && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genXOR1(lhs.gp, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_XOR && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genXOR1(lhs.mem, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_XOR && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genXOR1(lhs.gp, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_XOR && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genXOR1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_XOR && lhs.isGp() && operand->isInt()) {
            genXOR1(lhs.gp, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_XOR && lhs.isMem() && operand->isInt()) {
            genXOR1(lhs.mem, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genLAND1(lhs.gp, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genLAND1(lhs.mem, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genLAND1(lhs.gp, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genLAND1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isGp() && operand->isInt()) {
            genLAND1(lhs.gp, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LAND && lhs.isMem() && operand->isInt()) {
            genLAND1(lhs.mem, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genLOR1(lhs.gp, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genLOR1(lhs.mem, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genLOR1(lhs.gp, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genLOR1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isGp() && operand->isInt()) {
            genLOR1(lhs.gp, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LOR && lhs.isMem() && operand->isInt()) {
            genLOR1(lhs.mem, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genAND1(lhs.gp, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genAND1(lhs.mem, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genAND1(lhs.gp, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genAND1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isGp() && operand->isInt()) {
            genAND1(lhs.gp, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_AND && lhs.isMem() && operand->isInt()) {
            genAND1(lhs.mem, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genOR1(lhs.gp, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genOR1(lhs.mem, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genOR1(lhs.gp, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genOR1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isGp() && operand->isInt()) {
            genOR1(lhs.gp, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_OR && lhs.isMem() && operand->isInt()) {
            genOR1(lhs.mem, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genNEQ1(lhs.gp, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genNEQ1(lhs.mem, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genNEQ1(lhs.gp, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genNEQ1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isGp() && operand->isInt()) {
            genNEQ1(lhs.gp, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_NEQ && lhs.isMem() && operand->isInt()) {
            genNEQ1(lhs.mem, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genEQ1(lhs.gp, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genEQ1(lhs.mem, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genEQ1(lhs.gp, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genEQ1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isGp() && operand->isInt()) {
            genEQ1(lhs.gp, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_EQ && lhs.isMem() && operand->isInt()) {
            genEQ1(lhs.mem, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genLE1(lhs.gp, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genLE1(lhs.mem, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genLE1(lhs.gp, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genLE1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isGp() && operand->isInt()) {
            genLE1(lhs.gp, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LE && lhs.isMem() && operand->isInt()) {
            genLE1(lhs.mem, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genLT1(lhs.gp, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genLT1(lhs.mem, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genLT1(lhs.gp, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genLT1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isGp() && operand->isInt()) {
            genLT1(lhs.gp, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_LT && lhs.isMem() && operand->isInt()) {
            genLT1(lhs.mem, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genMOD1(lhs.gp, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genMOD1(lhs.mem, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genMOD1(lhs.gp, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genMOD1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isGp() && operand->isInt()) {
            genMOD1(lhs.gp, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MOD && lhs.isMem() && operand->isInt()) {
            genMOD1(lhs.mem, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFDIV1(lhs.xmm, locate(operand->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFDIV1(lhs.mem, locate(operand->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFDIV1(lhs.xmm, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFDIV1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isXmm() && operand->isDouble()) {
            genFDIV1(lhs.xmm, contents(operand->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FDIV && lhs.isMem() && operand->isDouble()) {
            genFDIV1(lhs.mem, contents(operand->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFMUL1(lhs.xmm, locate(operand->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFMUL1(lhs.mem, locate(operand->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFMUL1(lhs.xmm, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFMUL1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isXmm() && operand->isDouble()) {
            genFMUL1(lhs.xmm, contents(operand->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FMUL && lhs.isMem() && operand->isDouble()) {
            genFMUL1(lhs.mem, contents(operand->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFSUB1(lhs.xmm, locate(operand->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFSUB1(lhs.mem, locate(operand->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFSUB1(lhs.xmm, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFSUB1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isXmm() && operand->isDouble()) {
            genFSUB1(lhs.xmm, contents(operand->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FSUB && lhs.isMem() && operand->isDouble()) {
            genFSUB1(lhs.mem, contents(operand->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFADD1(lhs.xmm, locate(operand->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFADD1(lhs.mem, locate(operand->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFADD1(lhs.xmm, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFADD1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isXmm() && operand->isDouble()) {
            genFADD1(lhs.xmm, contents(operand->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_FADD && lhs.isMem() && operand->isDouble()) {
            genFADD1(lhs.mem, contents(operand->asDouble()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genDIV1(lhs.gp, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genDIV1(lhs.mem, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genDIV1(lhs.gp, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genDIV1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isGp() && operand->isInt()) {
            genDIV1(lhs.gp, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_DIV && lhs.isMem() && operand->isInt()) {
            genDIV1(lhs.mem, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genMUL1(lhs.gp, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genMUL1(lhs.mem, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genMUL1(lhs.gp, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genMUL1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isGp() && operand->isInt()) {
            genMUL1(lhs.gp, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_MUL && lhs.isMem() && operand->isInt()) {
            genMUL1(lhs.mem, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genSUB1(lhs.gp, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genSUB1(lhs.mem, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genSUB1(lhs.gp, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genSUB1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isGp() && operand->isInt()) {
            genSUB1(lhs.gp, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_SUB && lhs.isMem() && operand->isInt()) {
            genSUB1(lhs.mem, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genADD1(lhs.gp, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genADD1(lhs.mem, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genADD1(lhs.gp, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genADD1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isGp() && operand->isInt()) {
            genADD1(lhs.gp, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::BinOp::BO_ADD && lhs.isMem() && operand->isInt()) {
            genADD1(lhs.mem, contents(operand->asInt()), _, runtime);
            return;
        }
        throw CodeGenerationError("Bad argument for reduced binary operation generator");
    }


    void CodeGenerator::genUnOp(IR::UnOp::Type type, RegOrMem lhs, IR::Atom const *const operand, X86Assembler &_, MvmRuntime& runtime) {
        if (type == IR::UnOp::UO_FNEG && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFNEG1(lhs.xmm, locate(operand->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_FNEG && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genFNEG1(lhs.mem, locate(operand->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_FNEG && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFNEG1(lhs.xmm, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_FNEG && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genFNEG1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_FNEG && lhs.isXmm() && operand->isDouble()) {
            genFNEG1(lhs.xmm, contents(operand->asDouble()), _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_FNEG && lhs.isMem() && operand->isDouble()) {
            genFNEG1(lhs.mem, contents(operand->asDouble()), _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_NOT && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genNOT1(lhs.gp, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_NOT && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genNOT1(lhs.mem, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_NOT && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genNOT1(lhs.gp, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_NOT && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genNOT1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_NOT && lhs.isGp() && operand->isInt()) {
            genNOT1(lhs.gp, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_NOT && lhs.isMem() && operand->isInt()) {
            genNOT1(lhs.mem, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_NEG && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genNEG1(lhs.gp, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_NEG && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genNEG1(lhs.mem, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_NEG && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genNEG1(lhs.gp, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_NEG && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genNEG1(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_NEG && lhs.isGp() && operand->isInt()) {
            genNEG1(lhs.gp, contents(operand->asInt()), _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_NEG && lhs.isMem() && operand->isInt()) {
            genNEG1(lhs.mem, contents(operand->asInt()), _, runtime);
            return;
        }
        //NOT PREGENERATED
        if (type == IR::UnOp::UO_CAST_I2D && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genCAST_I2D(lhs.xmm, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_CAST_I2D && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isGp()) {
            genCAST_I2D(lhs.mem, locate(operand->asVariable()).gp, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_CAST_I2D && lhs.isXmm() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genCAST_I2D(lhs.xmm, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_CAST_I2D && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genCAST_I2D(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }


        if (type == IR::UnOp::UO_CAST_D2I && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genCAST_D2I(lhs.gp, locate(operand->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_CAST_D2I && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isXmm()) {
            genCAST_D2I(lhs.mem, locate(operand->asVariable()).xmm, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_CAST_D2I && lhs.isGp() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genCAST_D2I(lhs.gp, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        if (type == IR::UnOp::UO_CAST_D2I && lhs.isMem() && operand->isVariable() && locate(operand->asVariable()).isMem()) {
            genCAST_D2I(lhs.mem, locate(operand->asVariable()).mem, _, runtime);
            return;
        }
        throw CodeGenerationError("Bad argument for unary operation generator");
    }

}
