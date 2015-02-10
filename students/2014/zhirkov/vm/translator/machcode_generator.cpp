->#include "machcode_generator.h"
#include <queue>


#include <algorithm>

#define D2I(d) (*((int64_t*)(&d)))
#define FOR_REGTYPES(reg, actGP, actXMM) if ((reg)->isGp())\
     { X86GpReg const&  gpReg = *((X86GpReg const*) reg);  {actGP }}\
     else { X86XmmReg const&  xmmReg = *((X86XmmReg const*) reg); {actXMM }}

#define FOR_INT_DOUBLE(type, actip, actd)  switch (type) {\
    case IR::VT_Int: case IR::VT_Ptr:{actip};    break; \
    case IR::VT_Double: {actd}; break;\
    default: throw std::invalid_argument("Variable has unsupported type");}

#define ASM_DEBUG(expr)

namespace mathvm {


    const X86GpReg allocableRegs[] = {
            rbx, rcx, rdx, rdi, rsi, r8, r9, r11, r12, r13, r14, r15
    };
    const X86GpReg tempGpRegs[] = {
            rax
    };

//    const X86XmmReg tempXmmRegs[] = {
//            xmm12, xmm13
//    };
    const X86XmmReg allocableXmmRegs[] = {
            xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm9, xmm10, xmm11
    };

    const X86GpReg integerArgumentsRegs[] = {
            rdi, rsi, rdx, rcx, r8, r9
    };

    const X86XmmReg sseArgumentsRegs[] = {
            xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7
    };


    struct RegComp {
        bool operator()(X86GpReg const &left, X86GpReg const &right) {
            return left.getId() < right.getId();
        }

        bool operator()(X86XmmReg const &left, X86XmmReg const &right) {
            return left.getId() < right.getId();
        }
    };


    MachCodeGenerator::FunctionDescriptor::FunctionDescriptor(IR::SimpleIr const &ir, IR::RegAllocInfo const &regAllocInfo, IR::FunctionRecord const &f, asmjit::Label const &label)
            : label(label), function(f) {

        std::vector<size_t> nextIntArg;
        std::vector<size_t> nextSseArg;
        for (size_t i = 0; i < INTEGER_ARGUMENTS_REGS_LENGTH; i++)
            nextIntArg.push_back(std::find(allocableRegs, allocableRegs + ALLOCABLE_REGS_COUNT, integerArgumentsRegs[i]) - allocableRegs);
        for (size_t i = 0; i < SSE_ARGUMENTS_REGS_LENGTH; i++)
            nextSseArg.push_back(std::find(allocableXmmRegs, allocableXmmRegs + ALLOCABLE_REGS_COUNT, sseArgumentsRegs[i]) - allocableXmmRegs);


        std::set<size_t> freeGpRegs;
        std::set<size_t> freeXmmRegs;

        for (size_t i = 0; i < INTEGER_ARGUMENTS_REGS_LENGTH; i++) freeGpRegs.insert(i);
        for (size_t i = 0; i < SSE_ARGUMENTS_REGS_LENGTH; i++) freeXmmRegs.insert(i);

        bool assignedGp[ALLOCABLE_REGS_COUNT] = {false};
        bool assignedXmm[ALLOCABLE_REGS_COUNT] = {false};

        for (size_t i = 0; i < f.arguments(); ++i) {
            auto argId = f.argument(i);
            uint64_t vreg = regAllocInfo.regAlloc.at(argId);
            switch (argumentClassForType(ir.varMeta[argId].type)) {
                case AC_INTEGER:
                    if (!nextIntArg.empty()) {
                        auto reg = nextIntArg.back();
                        nextIntArg.pop_back();
                        regsGp[vreg] = reg;
                        assignedGp[vreg] = true;
                        freeGpRegs.erase(vreg);
                    }
                    break;
                case AC_SSE:
                    if (!nextSseArg.empty()) {
                        auto reg = nextSseArg.back();
                        nextSseArg.pop_back();
                        regsXmm[vreg] = reg;
                        assignedXmm[vreg] = true;
                        freeXmmRegs.erase(vreg);
                    }
                    break;
                default:
                    throw std::invalid_argument("Unknown argument class");
            }
        }

        auto freeGpIt = freeGpRegs.cbegin();
        auto freeXmmIt = freeXmmRegs.cbegin();
        for (size_t i = 0; i < ALLOCABLE_REGS_COUNT; i++) {
            if (!assignedGp[i]) {
                regsGp[i] = *freeGpIt;
                freeGpIt++;
            }
            if (!assignedXmm[i]) {
                regsXmm[i] = *freeXmmIt;
                freeXmmIt++;
            }
        }
    }


    void MachCodeGenerator::prologue(IR::FunctionRecord const &f) {
        const size_t locals = f.memoryCells.size();
        const size_t argcount = f.parametersIds.size() + f.refParameterIds.size();
        if (locals > 0 || argcount > 0) {
            _.push(rbp);
            _.mov(rbp, rsp);
        }
        if (locals > 0) _.sub(rsp, locals);
    }

    void MachCodeGenerator::epilogue(IR::FunctionRecord const &f) {
        const size_t locals = f.memoryCells.size();
        const size_t argcount = f.parametersIds.size() + f.refParameterIds.size();
        if (locals > 0 || argcount > 0) {
            _.mov(esp, ebp);
            _.pop(ebp);
        }
        _.ret();
    }

//    static bool isArgument(IR::VarId id, IR::FunctionRecord const &f) {
//        for (size_t i = 0; i < f.arguments(); i++)
//            if (f.argument(i) == id) return true;
//        return false;
//    }

    void MachCodeGenerator::femms() {
        _.femms();
        _currentMode = CM_NORMAL;
    }

    void MachCodeGenerator::emitCall(IR::Call const &call) {
        if (_currentMode == CM_X87) femms();
    }


    void MachCodeGenerator::visit(const IR::Return *const expr) { //ok
        auto a = expr->atom;
        if (a->isInt() || a->isPtr())
            atomToReg(rax, a);
        if (a->isDouble())
            atomToReg(xmm0, a);

        if (a->isVariable()) {
            IR::Variable const *var = a->asVariable();
            FOR_INT_DOUBLE(typeOf(var->id),
                    atomToReg(rax, a);,
                    atomToReg(xmm0, a);)
        }
    }


    void MachCodeGenerator::visit(const IR::Assignment *const expr) {
        FunctionDescriptor::MemArg const *memArg = functions.at(_currentFunction)->isMemArg(expr->var->id);
        if (memArg)
            _nextAssignment = RegOrMem(X86Mem(rbp, memArg->ebpOffset));
        else {
            auto vreg = virtRegOf(expr->var->id);
            FOR_INT_DOUBLE(typeOf(expr->var->id),
                    _nextAssignment = RegOrMem(gpFromVirt(vreg));,
                    _nextAssignment = RegOrMem(xmmFromVirt(vreg));)
            expr->value->visit(this);
        }

//        X86Reg const *const lhsReg = regOfVar(expr->var->id);
//        FOR_REGTYPES(lhsReg,
//                {
//                    if (expr->value->isAtom()) atomToReg(gpReg, expr->value->asAtom());
//                    else if (expr->value->isUnOp()) {
//                        IR::UnOp const *const unop = expr->value->asUnOp();
//                        switch (unop->type) {
//                            case IR::UnOp::UO_CAST_D2I: {
//                                atomToReg(tempXmmRegs[0], unop->operand);
//                                _.cvtsd2si(gpReg, tempXmmRegs[0]);
//                                break;
//                            };
//                            case IR::UnOp::UO_NEG: {
//                                atomToReg(gpReg, unop->operand);
//                                _.neg(gpReg);
//                                break;
//                            };
//                            case IR::UnOp::UO_NOT: {
//                                atomToReg(gpReg, unop->operand);
//                                _.neg(gpReg);
//                                break;
//                            }
//                            default:
//                                throw std::invalid_argument("Invalid unary operation type");
//                        }
//                    }
//                    else if (expr->value->isBinOp()) {
//
//                    }
//                }, {
//            if (expr->value->isAtom()) atomToReg(xmmReg, expr->value->asAtom());
//            else if (expr->value->isUnOp()) {
//                IR::UnOp const *const unop = expr->value->asUnOp();
//                switch (unop->type) {
//                    case IR::UnOp::UO_CAST_I2D: {
//                        atomToReg(rax, unop->operand);
//                        _.cvtsi2sd(xmmReg, rax);
//                        break;
//                    }
//                    case IR::UnOp::UO_FNEG: {
//                        atomToReg(xmmReg, unop->operand);
//                        _.mov(tempGpRegs[0], 0x4045000000000000);
//                        _.movq(tempXmmRegs[1], rax);
//                        _.xorpd(xmmReg, tempXmmRegs[1]);
//                        _.movq(gpReg, xmm0);
//                        break;
//                    };
//                    default:
//                        throw std::invalid_argument("Invalid unary operation type");
//                }
//            }
//
//        })
    }

    void MachCodeGenerator::visit(const IR::FunctionRecord *const expr) {
        _currentFunction = expr;

        prologue(*expr);
        functions[expr] = new FunctionDescriptor(ir, regAllocInfo[expr->id], *expr, _.newLabel());

        auto blocks = blocksPostOrder(expr->entry);
        for (auto b : blocks) visit(b);
        epilogue(*expr);
    }


    void MachCodeGenerator::visit(const IR::Block *const expr) {
        blocks[expr] = BlockDescriptor {_.newLabel()};
        for (auto st: expr->contents)
            st->visit(this);
    }


    //assumes it is not an argument stored into stack
    X86Reg const *MachCodeGenerator::regOfVar(IR::VarId id) const {
        FOR_INT_DOUBLE(typeOf(id),
                return &gpFromVirt(virtRegOf(id));,
                return &xmmFromVirt(virtRegOf(id));)
    }


    void MachCodeGenerator::atomToReg(X86Reg const &dest, IR::Atom const *const atom) {
        switch (atom->getType()) {
            case IR::IrElement::IT_Variable: {
                const IR::Variable *const var = atom->asVariable();
                auto const *const memArg = functions[_currentFunction]->isMemArg(var->id);
                if (memArg) {
                    FOR_REGTYPES(&dest,
                            {
                                _.mov(gpReg, ptr(rbp, memArg->ebpOffset, 1));
                            },
                            {
                                _.movq(xmmReg, ptr(rbp, memArg->ebpOffset, 1));
                            });
                }
                else { //not a memory argument
                    FOR_REGTYPES(&dest,
                            FOR_INT_DOUBLE(
                                    typeOf(var->id),
                                    {
                                            _.mov(gpReg, gpFromVirt(virtRegOf(var->id)));
                                    },
                                    {
                                            _.movq(gpReg, xmmFromVirt(virtRegOf(var->id)));
                                    }),
                            FOR_INT_DOUBLE(
                                    typeOf(var->id),
                                    {
                                            _.movq(xmmReg, gpFromVirt(virtRegOf(var->id)));
                                    },
                                    {
                                            _.movq(xmmReg, xmmFromVirt(virtRegOf(var->id)));
                                    })
                    )
                }

                break;
            };
            case IR::IrElement::IT_Int: {
                FOR_REGTYPES(&dest,
                        _.mov(gpReg, atom->asInt()->value);, {
                    _.mov(tempGpRegs[0], atom->asInt()->value);
                    _.movq(xmmReg, tempGpRegs[0]);
                })
                break;
            }
            case IR::IrElement::IT_Double: {
                FOR_REGTYPES(&dest,
                        _.mov(gpReg, D2I(atom->asDouble()->value));, {
                    _.mov(tempGpRegs[0], D2I(atom->asDouble()->value));
                    _.movq(xmmReg, tempGpRegs[0]);
                });
                break;
            }
            case IR::IrElement::IT_Ptr: {
                IR::Ptr const *const ptr = atom->asPtr();

                FOR_REGTYPES(&dest, {
                    if (ptr->isPooledString) _.mov(gpReg, imm_ptr((void *) ir.pool[ptr->value].c_str()));
                    else _.mov(gpReg, ptr->value);
                }, {
                    if (ptr->isPooledString) {
                        _.mov(tempGpRegs[0], imm_ptr((void *) ir.pool[ptr->value].c_str()));
                        _.movq(xmmReg, tempGpRegs[0]);
                    }
                    else {
                        _.mov(tempGpRegs[0], ptr->value);
                        _.movq(xmmReg, tempGpRegs[0]);
                    }
                });
                break;
            }
            case IR::IrElement::IT_ReadRef:
                throw std::invalid_argument("readref not supported yet");
            default:
                throw std::invalid_argument("attempt to move non-atom into register");
        };
    }

    void MachCodeGenerator::visit(IR::Variable const *const var) {
        auto const *const memArg = functions[_currentFunction]->isMemArg(var->id);
        if (memArg) {
            RegOrMemMatch(_nextAssignment, {
                _.mov(tempGpRegs[0], mem);
                _.mov(X86Mem(rbp, memArg->ebpOffset), tempGpRegs[0]);
            }, {
                _.mov(gpReg, ptr(rbp, memArg->ebpOffset, 1));
            }, {
                _.movq(xmmReg, ptr(rbp, memArg->ebpOffset, 1));
            }
            )
        }
        else { //not a memory argument
            RegOrMemMatch(_nextAssignment,
                    {
                        FOR_INT_DOUBLE(
                                typeOf(var->id),
                                {
                                        _.mov(mem, gpFromVirt(virtRegOf(var->id)));
                                },
                                {
                                        _.movq(mem, xmmFromVirt(virtRegOf(var->id)));
                                })
                    },
                    {
                        FOR_INT_DOUBLE(
                                typeOf(var->id),
                                {
                                        _.mov(gpReg, gpFromVirt(virtRegOf(var->id)));
                                },
                                {
                                        _.movq(gpReg, xmmFromVirt(virtRegOf(var->id)));
                                })
                    },
                    {
                        FOR_INT_DOUBLE(
                                typeOf(var->id),
                                {
                                        _.movq(xmmReg, gpFromVirt(virtRegOf(var->id)));
                                },
                                {
                                        _.movq(xmmReg, xmmFromVirt(virtRegOf(var->id)));
                                })
                    })
        }
    }

    void MachCodeGenerator::visit(IR::Phi const *const expr) {
        throw std::bad_function_call();
    }

    void MachCodeGenerator::visit(IR::Int const *const expr) {
        RegOrMemMatch(_nextAssignment,
                _.mov(mem, Imm(expr->value)),
                _.mov(gpReg, Imm(expr->value)),
                _.mov(tempGpRegs[0], Imm(expr->value));
                _.movq(xmmReg, tempGpRegs[0]));
    }

    void MachCodeGenerator::visit(IR::Double const *const expr) {
        const int64_t value = D2I(expr->value);
        RegOrMemMatch(_nextAssignment,
                _.mov(mem, value),
                _.mov(gpReg, value),
                _.mov(tempGpRegs[0], value);
                _.movq(xmmReg, tempGpRegs[0]))
    }

    void MachCodeGenerator::visit(IR::Ptr const *const expr) {
        const uint64_t value = (expr->isPooledString) ? (uint64_t) (&_stringPool[expr->value]) : expr->value;
        RegOrMemMatch(_nextAssignment,
                _.mov(mem, value),
                _.mov(gpReg, value),
                _.mov(tempGpRegs[0], value);
                _.movq(xmmReg, tempGpRegs[0]));
    }

    void MachCodeGenerator::visit(IR::ReadRef const *const expr) {

    }


    void MachCodeGenerator::visit(const IR::BinOp *const expr) {

    }

    void MachCodeGenerator::visit(const IR::UnOp *const unop) {
        switch (unop->type) {
            case IR::UnOp::UO_CAST_D2I: {
                RegOrMemMatch(_nextAssignment,
                {atomToReg([0], unop->operand);
                    _.cvtsd2si(tempGpRegs[0], mem);},{},{});



                atomToReg(tempXmmRegs[0], unop->operand);
                _.cvtsd2si(gpReg, tempXmmRegs[0]);
                break;
            };
            case IR::UnOp::UO_NEG: {
                atomToReg(gpReg, unop->operand);
                _.neg(gpReg);
                break;
            };
            case IR::UnOp::UO_NOT: {
                atomToReg(gpReg, unop->operand);
                _.neg(gpReg);
                break;
            }
            default:
                throw std::invalid_argument("Invalid unary operation type");
        }
    }

    void MachCodeGenerator::visit(const IR::Call *const expr) {

    }

    void MachCodeGenerator::visit(const IR::Print *const expr) {

    }

    void MachCodeGenerator::visit(const IR::JumpAlways *const expr) {

    }

    void MachCodeGenerator::visit(const IR::JumpCond *const expr) {

    }

    void MachCodeGenerator::visit(const IR::WriteRef *const expr) {

    }


    bool CodeGenErrorHandler::handleError(Error code, const char *message) {
        _debug << message << std::endl;
//        throw std::invalid_argument(message);
        return true;
    }


    void *MachCodeGenerator::translate() {
        _debug << "\n-------------------------------\n   Code generation has started \n-------------------------------\n";
//        CodeGenErrorHandler handler(_debug);
        StringLogger logger;
//        _.setErrorHandler(&handler);
        _.setLogger(&logger);
        for (auto f: ir.functions) visit(f);

        void *result = _.make();
        std::cerr << "Logged:" << logger.getString() << std::endl;
        if (result) return result;
        std::cerr << ErrorUtil::asString(_.getError()) << std::endl;
        return NULL;
    }

    void MachCodeGenerator::FunctionDescriptor::setupMemArgs(IR::SimpleIr const &ir) {
        int32_t memRegs = 0;
        size_t intArgsCount = 0, sseArgsCount = 0;

        for (size_t i = 0; i < function.arguments(); ++i) {
            auto argId = function.argument(i);
            switch (argumentClassForType(ir.varMeta[argId].type)) {
                case AC_INTEGER:
                    if (intArgsCount >= INTEGER_ARGUMENTS_REGS_LENGTH)
                        memArgs.push_back(MemArg {argId, (1 + memRegs++) * 8, AC_INTEGER});
                    intArgsCount++;
                    break;
                case AC_SSE:
                    if (sseArgsCount >= SSE_ARGUMENTS_REGS_LENGTH)
                        memArgs.push_back(MemArg {argId, (1 + memRegs++) * 8, AC_SSE});
                    sseArgsCount++;
                    break;
            }
        }
    }

    X86XmmReg const &MachCodeGenerator::xmmFromVirt(uint64_t vreg) const {
        return allocableXmmRegs[functions.at(_currentFunction)->regsXmm[vreg]];
    }

    X86GpReg const &MachCodeGenerator::gpFromVirt(uint64_t vreg) const {
        return allocableRegs[functions.at(_currentFunction)->regsGp[vreg]];
    }
}