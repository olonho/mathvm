#pragma once

#include <map>
#include "../../../../../libs/asmjit/asmjit.h"
#include "../ir/live_analyzer.h"
#include "../ir/reg_allocator.h"
#include "../ir/transformations/typederiver.h"

#include "mathvm_runtime.h"

#define D2I(d) (*((int64_t*)(&d)))


namespace mathvm {
    using namespace asmjit;
    using namespace asmjit::x86;

    struct MvmRuntime;

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


    extern const X86GpReg allocableRegs[ALLOCABLE_REGS_COUNT];
    extern const X86GpReg tempGpRegs[1];

    extern const X86GpReg tempOneByteRegs[1];

    extern const X86XmmReg tempXmmRegs[2];
    extern const X86XmmReg allocableXmmRegs[ALLOCABLE_REGS_COUNT];

    extern const X86GpReg integerArgumentsRegs[INTEGER_ARGUMENTS_REGS_LENGTH];
    extern const X86XmmReg sseArgumentsRegs[SSE_ARGUMENTS_REGS_LENGTH];


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

        bool isGp() {
            return type == RM_GPREG;
        }

        bool isXmm() {
            return type == RM_XMMREG;
        }

        bool isMem() {
            return type == RM_MEM;
        }
    };

#define RegOrMemMatch(rm, ifmem, ifgp, ifxmm) switch (rm.type) {\
    case RegOrMem::RM_GPREG: { X86GpReg  const& gpReg = rm.gp; ifgp ; break;}\
    case RegOrMem::RM_XMMREG: { X86XmmReg const& xmmReg = rm.xmm; ifxmm ; break;}\
    case RegOrMem::RM_MEM: { X86Mem const& mem = rm.mem; ifmem ; break;}\
    default: throw std::invalid_argument("RegOrMem instance uninitialized"); break;\
}
#define RegOrMemMatchNoRedef(rm, ifmem, ifgp, ifxmm) switch ((rm).type) {\
    case RegOrMem::RM_GPREG: { ifgp ; break;}\
    case RegOrMem::RM_XMMREG: { ifxmm ; break;}\
    case RegOrMem::RM_MEM: { ifmem;  break;}\
    default: throw std::invalid_argument("RegOrMem instance uninitialized"); break;\
}


    struct CodeGenerator : public IR::IrVisitor<void> {
        friend class Info;

        struct Info {
            std::vector<size_t> nextIntArg;
            std::vector<size_t> nextSseArg;

            Info();
        };

        static const Info info;

        struct BlockDescriptor {
            asmjit::Label label;
        };
        enum ArgumentClass {
            AC_INTEGER,
            AC_SSE,
            AC_MEM
        };


        struct Function {
            struct Argument {
                size_t idx;
                IR::VarId var;
                IR::VarType type;
                ArgumentClass class_;
                X86GpReg gpReg;
                X86XmmReg xmmReg;
                int32_t memArgIdx;

                Argument(size_t idx, IR::VarId var, IR::VarType type, int32_t memArgIdx)
                        : idx(idx),
                          var(var),
                          type(type),
                          class_(AC_MEM),
                          memArgIdx(memArgIdx) {
                }

                Argument(size_t idx, IR::VarId var, IR::VarType type, X86GpReg const &gpReg)
                        : idx(idx),
                          var(var),
                          type(type),
                          class_(CodeGenerator::argumentClassForType(type)),
                          gpReg(gpReg) {
                }

                Argument(size_t idx, IR::VarId var, IR::VarType type, X86XmmReg const &xmmReg)
                        : idx(idx),
                          var(var),
                          type(type),
                          class_(CodeGenerator::argumentClassForType(type)),
                          xmmReg(xmmReg) {
                }

                int32_t getRbpOffset() const {
                    return (memArgIdx + 1) * 8;
                }
            };


            Function(CodeGenerator &generator, IR::FunctionRecord const &f, asmjit::Label const &label);

            asmjit::Label label;
            std::vector<Argument> arguments;
            size_t regsGp[ALLOCABLE_REGS_COUNT];  //vreg -> index of allocableReg
            size_t regsXmm[ALLOCABLE_REGS_COUNT]; //vreg -> index of allocableXmmReg
            IR::FunctionRecord const &function;
            CodeGenerator const &generator;
            std::vector<IR::Block const*> const& orderedBlocks ;
            Argument const *argumentForVar(IR::VarId id) const {
                for (auto &a: arguments) if (a.var == id) return &a;
                return NULL;
            }

            bool hasMemArgs() const {
                for (Argument const &arg: arguments) if (arg.class_ == AC_MEM) return true;
                return false;
            }

        private:
            void setupRegisterMapping();

            void setupArguments();
        };


    private:
        const IR::SimpleIr &_ir;
        MvmRuntime &_runtime;
        X86Assembler _;
        std::map<IR::Block const *, BlockDescriptor> _blocks;
        std::vector<Function *> _functions;

        IR::LiveInfo const *const _liveInfo;
        const IR::GlobalRegAllocInfo regAllocInfo;

        std::ostream &_debug;
        IR::IrPrinter _irPrinter;
        IR::TypeDeriver _deriver;


        IR::FunctionRecord const *_currentFunction;
        RegOrMem _nextAssignment;
        IR::VarId _nextAssignedVreg;


    public:

        CodeGenerator(IR::SimpleIr const &ir, MvmRuntime &runtime, std::ostream &debug);

        virtual ~CodeGenerator() {
        }


        void *translate();

        virtual void visit(IR::ReadRef const *const expr);

        virtual void visit(IR::Ptr const *const expr);

        virtual void visit(IR::Double const *const expr);

        virtual void visit(IR::Int const *const expr);

        virtual void visit(IR::Phi const *const expr) override;

        virtual void visit(IR::Variable const *const expr);

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

        void prologue(IR::FunctionRecord const &f);

        void epilogue(IR::FunctionRecord const &f);

        void genAssign(RegOrMem const &from, RegOrMem const &to);

        void genBinOp(IR::BinOp::Type type, RegOrMem lhs, IR::Atom const *const fst, IR::Atom const *const snd, X86Assembler &_);


        void initBlock(IR::Block const *const block) {
            _blocks[block] = BlockDescriptor {_.newLabel()};
        }

        void initFunction(IR::FunctionRecord const *const f) {
            _functions[f->id] = new Function(*this, *f, _.newLabel());
        }

        uint64_t virtRegOf(uint64_t funId, IR::VarId id) const {
            return regAllocInfo.at(funId).regAlloc.at(id);
        }

        uint64_t virtRegOf(IR::VarId id) const {
            return virtRegOf(_currentFunction->id, id);
        }

        X86GpReg const &gpFromVirt(uint64_t vreg) const;

        X86XmmReg const &xmmFromVirt(uint64_t vreg) const;

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
            return argumentClassForType(_ir.varMeta[id].type);
        }

        IR::VarType typeOf(IR::VarId id) const {
            return _ir.varMeta[id].type;
        }


        RegOrMem locate(IR::VarId id) const;

        RegOrMem locate(IR::Variable const *var) const {
            return locate(var->id);
        }

        int64_t contents(IR::Ptr const *ptr) const;

        int64_t contents(IR::Int const *i) const {
            return i->value;
        }

        double contents(IR::Double const *d) const {
            return d->value;
        }

        IR::RegAllocInfo const &allocations() const {
            return regAllocInfo[_currentFunction->id];
        }

        std::map<IR::VarId, uint64_t> const &regAlloc() const {
            return regAllocInfo[_currentFunction->id].regAlloc;
        };

        bool isVarAlive(IR::VarId var, size_t position) const {
            return _liveInfo->data.at(_currentFunction->id)->varIntervals.at(var).contains(position);
        }

        bool isVarAlive(IR::VarId var, const IR::Statement *const statement) const {
            return isVarAlive(var, statement->num);
        }

        bool isRegAliveAfter(const IR::Statement *const statement, X86GpReg const &reg);

        bool isRegAliveAfter(const IR::Statement *const statement, X86XmmReg const &reg);

        void genReducedBinOp(IR::BinOp::Type, RegOrMem, IR::Atom const *const, X86Assembler &assembler);

        void genUnOp(IR::UnOp::Type, RegOrMem, IR::Atom const *const, X86Assembler &assembler);

    };

#pragma GCC visibility pop

};