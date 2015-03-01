#pragma once

#include <map>
#include "../../../../../libs/asmjit/asmjit.h"
#include "../ir/live_analyzer.h"
#include "../ir/reg_allocator.h"
#include "../ir/transformations/typederiver.h"

#include "mathvm_runtime.h"
#include "../exceptions.h"

#define D2I(d) (*((int64_t*)(&d)))


namespace mathvm {
    using namespace asmjit;
    using namespace asmjit::x86;

    struct MvmRuntime;

    struct CodeGenErrorHandler : public ErrorHandler {
        CodeGenErrorHandler(std::ostream &debug) : _debug(debug) {
        }

        virtual bool handleError(asmjit::Error code, const char* message)  ;

    private:
        std::ostream &_debug;
    };

#define ALLOCABLE_REGS_COUNT 12

#define INTEGER_ARGUMENTS_REGS_LENGTH 6

#define SSE_ARGUMENTS_REGS_LENGTH 8

    extern const X86GpReg callerSave[8];
    extern const X86GpReg calleeSave[5];


    extern const X86GpReg allocableRegs[ALLOCABLE_REGS_COUNT];
    extern const X86GpReg tempGpRegs[1];

    extern const X86GpReg tempOneByteRegs[1];

    extern const X86XmmReg tempXmmRegs[2];
    extern const X86XmmReg allocableXmmRegs[ALLOCABLE_REGS_COUNT];

    extern const X86GpReg integerArgumentsRegs[INTEGER_ARGUMENTS_REGS_LENGTH];
    extern const X86XmmReg sseArgumentsRegs[SSE_ARGUMENTS_REGS_LENGTH];

    struct RegComp {
        bool operator()(X86GpReg const& fst, X86GpReg const& snd) { return fst.getRegIndex() < snd.getRegIndex(); }
        bool operator()(X86XmmReg const& fst, X86XmmReg const& snd) { return fst.getRegIndex() < snd.getRegIndex(); }
    };


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

    struct Reference {
        enum Type {
            REF_IMM,
            REF_RM
        };
        const Reference::Type type;
        const RegOrMem rm;
        Reference(RegOrMem rm) : type(REF_RM), rm(rm), imm(0) {}
        Reference(int32_t imm): type(REF_IMM),  imm(imm) {}

        X86Mem asMemCell() const { return X86Mem(ptr(rbp, imm)); }
    private:

        const int32_t imm;
    };

    inline bool operator==(RegOrMem const &lhs, RegOrMem const &rhs) {
        if (lhs.type != rhs.type) return false;
        switch (lhs.type) {
            case RegOrMem::RM_GPREG:
                return lhs.gp == rhs.gp;
            case RegOrMem::RM_XMMREG:
                return lhs.xmm == rhs.xmm;
            case RegOrMem::RM_MEM:
                return lhs.mem == rhs.mem;
            case RegOrMem::RM_UNDEFINED:
                return true;
        }
        return false;
    }


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
        friend struct Function;
        struct Info {
            Info();
            std::vector<size_t> nextIntArg;
            std::vector<size_t> nextSseArg;
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
                const bool isRef;

                Argument(size_t idx, IR::VarId var, IR::VarType type, int32_t memArgIdx, bool const isRef)
                        : idx(idx),
                          var(var),
                          type(type),
                          class_(AC_MEM),
                          memArgIdx(memArgIdx), isRef(isRef) {
                }

                Argument(size_t idx, IR::VarId var, IR::VarType type, X86GpReg const &gpReg, bool const isRef)
                        : idx(idx),
                          var(var),
                          type(type),
                          class_(CodeGenerator::argumentClassForType(type)),
                          gpReg(gpReg), isRef(isRef) {
                }

                Argument(size_t idx, IR::VarId var, IR::VarType type, X86XmmReg const &xmmReg, bool const isRef)
                        : idx(idx),
                          var(var),
                          type(type),
                          class_(CodeGenerator::argumentClassForType(type)),
                          xmmReg(xmmReg), isRef(isRef) {
                }

                int32_t getRbpOffset() const {
                    return (memArgIdx + 1) * 8;
                }
                RegOrMem location() const {
                    switch(class_) {
                        case AC_INTEGER: return RegOrMem(gpReg);
                        case AC_SSE: return RegOrMem(xmmReg);
                        case AC_MEM: return RegOrMem(X86Mem(ptr(rbp, getRbpOffset())));
                        default: throw CodeGenerationError("Misplaced argument: can't find location");
                    }
                }
            };


            Function(CodeGenerator &generator, IR::Function const &f, asmjit::Label const &label);
            Function(CodeGenerator &generator, IR::Function const &f );

             const asmjit::Label label;
             std::vector<Argument> arguments;
             std::vector<int32_t> backedXmmArgsRegsRbpOffsets;
             size_t regsGp[ALLOCABLE_REGS_COUNT];  //vreg -> index of allocableReg
             size_t regsXmm[ALLOCABLE_REGS_COUNT]; //vreg -> index of allocableXmmReg
             IR::Function const &function;
            const IR::VarType returnType;
            CodeGenerator const &generator;
            std::vector<IR::Block const *> const &orderedBlocks;
            std::vector<X86GpReg> savedGpInPrologue;

            const bool isPredef;
            Argument const *argumentForVar(IR::VarId id) const {
                for (auto &a: arguments) if (a.var == id) return &a;
                return NULL;
            }

            bool hasMemArgs() const {
                for (Argument const &arg : arguments) if (arg.class_ == AC_MEM) return true;
                return false;
            }

            size_t memArgsCount() const {
                size_t counter = 0;
                for (Argument const &arg : arguments) if (arg.class_ == AC_MEM) counter++;
                return counter;
            }

            Argument const* refIndexOf(IR::VarId arg) const {
                for (auto& a : arguments)
                    if (a.isRef && a.var == arg) return &a;
                return NULL;
            }
            bool isStackAlignedAfterPrologue() const {
                return (function.memoryCells.size() + generator.hasStackFrame(function) + savedGpInPrologue.size()) % 2 == 1;
            }

            void setup() {
                if (function.id && !isPredef) savedGpInPrologue = generator.usedGpCalleeSave(function);
            }

             size_t sseRegArgsCount() const {
                 size_t count = 0;
                 for( auto& a: arguments)if (a.class_ == AC_SSE) count++;
                 return count;
             }

             bool isSseRegArg(IR::VarId id) const {
                 for(auto& a : arguments) if (a.class_ == AC_SSE & a.var == id) return true;
                 return false;
             }

        private:
            void setupRegisterMapping();

            void setupArguments();
        };


        IR::VarType typeOf(IR::VarId id) const {
            return _varMeta[id].type;
        }

        X86GpReg const &gpFromVirt(uint64_t funId, uint64_t vreg) const;

        X86GpReg const &gpFromVirt(uint64_t vreg) const;

        RegOrMem locate(IR::VarId id) const;

    private:
        const IR::SimpleIr &_ir;
        std::vector<IR::SimpleIr::VarMeta> _varMeta;
        MvmRuntime &_runtime;
        X86Assembler _;
        std::map<IR::Block const *, BlockDescriptor> _blocks;
        std::vector<Function *> _functions;


        std::ostream &_debug;
        IR::IrPrinter _irPrinter;
        IR::TypeDeriver _deriver;


        IR::Function const *_currentFunction;
        IR::Block const *_currentBlock;
        IR::Statement const *_currentStatement;
        RegOrMem _nextAssignment;
        IR::VarId _nextAssignedVreg;

        const IR::VarId gpAccVar;
        const IR::VarId doubleAccVar;

        const uint64_t gpAccVreg;
        const uint64_t doubleAccVreg;

    public:
        IR::LiveInfo const *const liveInfo;
        const IR::GlobalRegAllocInfo regAllocInfo;

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

        void genCall(Function const& f,std::vector<IR::Atom const*> const& args );

        virtual void visit(const IR::Print *const expr);

        virtual void visit(const IR::Function *const expr);

        virtual void visit(const IR::JumpAlways *const expr);

        virtual void visit(const IR::JumpCond *const expr);

        virtual void visit(const IR::WriteRef *const expr);


    private:

        IR::VarId makeVar(IR::VarType type) {
            _varMeta.push_back(IR::SimpleIr::VarMeta(_varMeta.size(), type));
            return _varMeta.size()-1;
        }

        void prologue(CodeGenerator::Function const &f);

        void epilogue(CodeGenerator::Function const &f);

        void genAssign(RegOrMem const &from, RegOrMem const &to);

        void genBinOp(IR::BinOp::Type type, RegOrMem lhs, IR::Atom const *const fst, IR::Atom const *const snd, X86Assembler &_, mathvm::MvmRuntime& runtime);

        void stackAlign16() {
            _.sub(rsp, 16); // why does 8 not work?
            _.and_(rsp, -2);
        }

        bool hasCalls(IR::Function const &f) const {
            for (auto st : f.entry->contents) if (st->isAssignment() && st->asAssignment()->value->isCall()) return true;
            return false;
        }

        bool hasStackFrame(IR::Function const &f) const;

        void initBlock(IR::Block const *const block) {
            _blocks[block] = BlockDescriptor {_.newLabel()};
        }

        void initFunction(IR::Function const *const f) {
            _functions[f->id] = new Function(*this, *f, _.newLabel());
        }

        IR::Block const * nextBlock() const;

        void genJmp(const IR::Block* dest) {
            if (dest != nextBlock()) _.jmp(_blocks[dest].label);
        }


        uint64_t virtRegOf(uint64_t funId, IR::VarId id) const {
            if (id == doubleAccVar) return doubleAccVreg;
            if (id == gpAccVar) return gpAccVreg;
            if (regAllocInfo.at(funId).stackAlloc.find(id) != regAllocInfo.at(funId).stackAlloc.cend() )
                throw BadIr("variable " + to_string(id) + " is stack allocated and should be addressed using readref");
            return regAllocInfo.at(funId).regAlloc.at(id);
        }

        uint64_t virtRegOf(IR::VarId id) const {
            return virtRegOf(_currentFunction->id, id);
        }

        X86XmmReg const &xmmFromVirt(uint64_t vreg) const;

        static ArgumentClass argumentClassForType(IR::VarType type) {
            switch (type) {
                case IR::VT_Int:
                case IR::VT_Ptr:
                    return AC_INTEGER;
                case IR::VT_Double:
                    return AC_SSE;
                default:
                    throw CodeGenerationError(std::string("argument class for type ") + IR::varTypeStr(type) + " is undefined");
            }
        }

        ArgumentClass argumentClass(IR::VarId id) {
            return argumentClassForType(_varMeta[id].type);
        }


        RegOrMem locate(IR::Variable const *var) const {
            return locate(var->id);
        }

        Reference locateRef(uint64_t funId, IR::VarId refId) const;

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
            if (var == doubleAccVar || var == gpAccVar) return false;
            return liveInfo->data.at(_currentFunction->id)->varIntervals.at(var).contains(position);
        }

        bool isVarAlive(IR::VarId var, const IR::Statement *const statement) const {
            return isVarAlive(var, statement->num);
        }

        bool isRegAliveAfter(const IR::Statement *const statement, X86GpReg const &reg);

        bool isRegAliveAfter(const IR::Statement *const statement, X86XmmReg const &reg);

        void genReducedBinOp(IR::BinOp::Type, RegOrMem, IR::Atom const *const, X86Assembler &assembler, mathvm::MvmRuntime& runtime);

        void genUnOp(IR::UnOp::Type, RegOrMem, IR::Atom const *const, X86Assembler &assembler, mathvm::MvmRuntime& runtime);


        static bool isCallerSave(X86GpReg const &reg);

        static bool isCalleeSave(X86GpReg const &reg);

        std::vector<X86GpReg> liveRegs(size_t statementIdx) const;

        std::vector<X86XmmReg> liveXmmRegs(size_t statementIdx) const;

        std::vector<X86GpReg> liveCallerSaveRegs(size_t statementIdx) const;

        std::vector<X86XmmReg> liveCallerSaveXmmRegs(size_t statementIdx) const {
            return liveXmmRegs(statementIdx);
        }

        std::set<X86GpReg, RegComp> usedGpRegs(IR::Function const &f) const;
        std::vector<X86GpReg> usedGpCalleeSave(IR::Function const& f ) const;
        std::vector<X86GpReg> usedXmmRegs(IR::Function const &f) const;

        std::vector<IR::Function const*> _predefFunctions;
        Function const* _f_printInt;
        Function const* _f_printDouble;
        Function const* _f_printStr;

        void setupPredefFunctions();

        CodeGenerator::Function const* makePredefFunction(void *address, IR::VarType arg, const std::string& name = "NONAME");
    };


#pragma GCC visibility pop

};

