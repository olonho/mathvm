#pragma once

#include <map>
#include "../../../../../libs/asmjit/asmjit.h"
#include "../ir/live_analyzer.h"
#include "../ir/reg_allocator.h"

namespace mathvm {
    using namespace asmjit;
    using namespace asmjit::x86;

    struct CodeGenErrorHandler : public ErrorHandler {
        CodeGenErrorHandler(std::ostream &debug) : _debug(debug) {
        }

        virtual bool handleError(Error code, const char *message);

    private:
        std::ostream &_debug;
    };

#define ALLOCABLE_REGS_COUNT 12

#define INTEGER_ARGUMENTS_REGS_LENGTH 6

#define SSE_ARGUMENTS_REGS_LENGTH 8

//    const X86GpReg &callerSave[] = {
//            rax, rdi, rsi, rdx, rcx, r8, r9, r11
//    };
//    const X86GpReg &calleeSaves[] = {
//            rbx, r12, r13, r14, r15
//    };

#pragma GCC visibility push(hidden)

    struct RegOrMem {
        enum Type {
            RM_GPREG,
            RM_XMMREG,
            RM_MEM,
            RM_UNDEFINED
        };
        Type type;

        X86Mem mem;
        X86XmmReg xmm;
        X86GpReg gp;

        RegOrMem(X86XmmReg rxmm) : type(RM_XMMREG), xmm(rxmm) {
        }

        RegOrMem(X86Mem mem) : type(RM_MEM), mem(mem) {
        }

        RegOrMem(X86GpReg gp) : type(RM_GPREG), gp(gp) {
        }

        RegOrMem() : type(RM_UNDEFINED) {
        }
    };

#define RegOrMemMatch(rm, ifmem, ifgp, ifxmm) switch (rm.type) {\
    case RegOrMem::RM_GPREG: { X86GpReg  const& gpReg = rm.gp; ifgp ; break;}\
    case RegOrMem::RM_XMMREG: { X86XmmReg const& xmmReg = rm.xmm; ifxmm ; break;}\
    case RegOrMem::RM_MEM: { X86Mem const& mem = rm.mem; ifmem ; break;}\
    default: throw std::invalid_argument("RegOrMem instance uninitialized");\
}


    struct MachCodeGenerator : public IR::IrVisitor<void> {


        struct BlockDescriptor {
            asmjit::Label label;
        };
        enum ArgumentClass {
            AC_INTEGER,
            AC_SSE
        };

        struct FunctionDescriptor {
            struct MemArg {
                IR::VarId id;
                int32_t ebpOffset;
                ArgumentClass argClass;
            };


            FunctionDescriptor(IR::SimpleIr const &ir, IR::RegAllocInfo const &regAllocInfo, IR::FunctionRecord const &f, asmjit::Label const &label);

            asmjit::Label label;
            std::vector<MemArg> memArgs;
            std::vector<X86Reg *> registers;
            size_t regsGp[ALLOCABLE_REGS_COUNT];  //vreg -> index of allocableReg
            size_t regsXmm[ALLOCABLE_REGS_COUNT];
            //vreg -> index of allocableXmmReg
            IR::FunctionRecord const &function;

            const MemArg *isMemArg(IR::VarId id) {
                for (MemArg const &memArg : memArgs)
                    if (memArg.id == id) return &memArg;
                return NULL;
            }

        private:
            void setupMemArgs(IR::SimpleIr const &ir);
        };

    private:
        const IR::GlobalRegAllocInfo &regAllocInfo;
        const IR::SimpleIr &ir;
        JitRuntime &runtime;
        X86Assembler _;
        std::map<IR::Block const *, BlockDescriptor> blocks;
        std::map<IR::FunctionRecord const *, FunctionDescriptor *> functions;
        enum CpuMode {
            CM_X87,
            CM_NORMAL
        };
        CpuMode _currentMode;
        std::ostream &_debug;

    public:
        virtual void visit(IR::ReadRef const *const expr);

        virtual void visit(IR::Ptr const *const expr);

        virtual void visit(IR::Double const *const expr);

        virtual void visit(IR::Int const *const expr);

        virtual void visit(IR::Phi const *const expr) override;

        virtual void visit(IR::Variable const *const expr);

        MachCodeGenerator(IR::SimpleIr const &ir, IR::GlobalRegAllocInfo const &regAllocInfo, JitRuntime &runtime, std::ostream &debug)
                : regAllocInfo(regAllocInfo),
                  ir(ir),
                  runtime(runtime),
                  _(&runtime),
                  _currentMode(CM_NORMAL),
                  _debug(debug),
                  _stringPool(ir.pool) {
        }


        void *translate();

        virtual void visit(const IR::BinOp *const expr);

        virtual void visit(const IR::UnOp *const expr);

        virtual void visit(const IR::Return *const expr);

        virtual void visit(const IR::Block *const expr);

        virtual void visit(const IR::Assignment *const expr);

        virtual void visit(const IR::Call *const expr);

        virtual void visit(const IR::Print *const expr);

        virtual void visit(const IR::FunctionRecord *const expr);

        virtual void visit(const IR::JumpAlways *const expr);

        virtual void visit(const IR::JumpCond *const expr);

        virtual void visit(const IR::WriteRef *const expr);


    private:
        const IR::SimpleIr::StringPool _stringPool;
        IR::FunctionRecord const *_currentFunction;

        RegOrMem _nextAssignment;

        void atomToReg(X86Reg const &dest, IR::Atom const *const atom);

        void prologue(IR::FunctionRecord const &f);

        void epilogue(IR::FunctionRecord const &f);

        X86Reg const *regOfVar(IR::VarId id) const;

        uint64_t virtRegOf(IR::VarId id) const {
            return regAllocInfo.at(_currentFunction->id).regAlloc.at(id);
        }

        X86GpReg const &gpFromVirt(uint64_t vreg) const;

        X86XmmReg const &xmmFromVirt(uint64_t vreg) const;

        void femms();

        void emitCall(IR::Call const &call);

        static ArgumentClass argumentClassForType(IR::VarType type) {
            switch (type) {
                case IR::VT_Int:
                case IR::VT_Ptr:
                    return AC_INTEGER;
                case IR::VT_Double:
                    return AC_SSE;
                default:
                    throw std::invalid_argument("argument class for such type is undefined");
            }
        }

        ArgumentClass argumentClass(IR::VarId id) {
            return argumentClassForType(ir.varMeta[id].type);
        }

        IR::VarType typeOf(IR::VarId id) const {
            return ir.varMeta[id].type;
        }
    };

#pragma GCC visibility pop
};