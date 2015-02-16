#include "machcode_generator.h"
#include "../ir/transformations/typederiver.h"
#include <queue>
#include <algorithm>
#include <stack>


#define FOR_REGTYPES(reg, actGP, actXMM) if ((reg)->isGp())\
     { X86GpReg const&  gpReg = *((X86GpReg const*) reg);  {actGP }}\
     else { X86XmmReg const&  xmmReg = *((X86XmmReg const*) reg); {actXMM }}

#define FOR_INT_DOUBLE(type, actip, actd)  switch (type) {\
    case IR::VT_Int: case IR::VT_Ptr:{actip};    break; \
    case IR::VT_Double: {actd}; break;\
    default: throw std::invalid_argument("Variable has unsupported type");}

#define FOR_ATOM(atom, ifint, ifdouble, ifptr, ifvar, ifreadref) switch ((atom)->getType()) {\
case IR::IrElement::IT_Variable: {const IR::Variable* const aVar = atom->asVariable(); ifvar; break;} \
case IR::IrElement::IT_Int: {const IR::Int* const aInt = atom->asInt(); ifint;  break; } \
case IR::IrElement::IT_Double: {const IR::Double* const aDouble = atom->asDouble(); ifdouble; break; }\
case IR::IrElement::IT_Ptr: {const IR::Ptr* const aPtr = atom->asPtr();ifptr ; break;}\
case IR::IrElement::IT_ReadRef: {const IR::ReadRef* const aReadRef = atom->asReadRef();ifreadref; break;};\
 default:throw std::invalid_argument("Not an atom");\
};

#define ERROR {throw std::invalid_argument("invalid instruction arguments"); }
#define UNSUPPORTED {throw std::invalid_argument("not yet supported"); }

namespace mathvm {


    const X86GpReg allocableRegs[] = {
            rbx, rcx, rdx, rdi, rsi, r8, r9, r11, r12, r13, r14, r15
    };
    const X86GpReg tempGpRegs[] = {
            rax
    };

    const X86GpReg tempOneByteRegs[]{
            al
    };

    const X86XmmReg tempXmmRegs[] = {
            xmm12, xmm13
    };
    const X86XmmReg allocableXmmRegs[] = {
            xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8, xmm9, xmm10, xmm11
    };

    const X86GpReg integerArgumentsRegs[] = {
            rdi, rsi, rdx, rcx, r8, r9
    };

    const X86XmmReg sseArgumentsRegs[] = {
            xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7
    };


    static void pushRegs(X86Assembler &_) {
        _.pushf();
        for (uint32_t i = 0; i < ALLOCABLE_REGS_COUNT; i++)
            _.push(allocableRegs[i]);

        for (uint32_t i = 0; i < 16; i++) {
            _.movq(rax, xmm(i));
            _.push(rax);
        }
    }

    static void popRegs(X86Assembler &_) {
        for (uint32_t i = 0; i < 16; i++) {
            _.pop(rax);
            _.movq(xmm(15 - i), rax);
        }
        for (uint32_t i = 0; i < ALLOCABLE_REGS_COUNT; i++)
            _.pop(allocableRegs[ALLOCABLE_REGS_COUNT - 1 - i]);

        _.popf();
    }

    MachCodeGenerator::Function::Function(IR::SimpleIr const &ir, IR::RegAllocInfo const &regAllocInfo, IR::FunctionRecord const &f, asmjit::Label const &label)
            : label(label), function(f) {
        size_t intArgs = 0, sseArgs = 0;
        uint32_t memArgs = 0;
        for (size_t i = 0; i < f.arguments(); ++i) {
            auto argId = f.argument(i);
            uint64_t vreg = regAllocInfo.regAlloc.at(argId);
            const IR::VarType type = ir.varMeta[argId].type;
            switch(MachCodeGenerator::argumentClassForType(type)) {
                case AC_INTEGER:
                    if (intArgs < INTEGER_ARGUMENTS_REGS_LENGTH)
                        arguments.push_back(Argument(i, argId, type, integerArgumentsRegs[intArgs]));
                    else
                        arguments.push_back(Argument(i, argId, type, memArgs++));
                    intArgs++;
                    break;
                case AC_SSE:
                    if (sseArgs < INTEGER_ARGUMENTS_REGS_LENGTH)
                        arguments.push_back(Argument(i, argId, type, sseArgumentsRegs[intArgs]));
                    else
                        arguments.push_back(Argument(i, argId, type, memArgs++));
                    sseArgs++;
                    break;
                case AC_MEM:
                    arguments.push_back(Argument(i, argId, type, memArgs++));
                    break;
            }
        }

        setupRegisterMapping();
    }



    void MachCodeGenerator::prologue(IR::FunctionRecord const &f) {
        const size_t locals = f.memoryCells.size();
        const size_t argcount = f.parametersIds.size() + f.refParameterIds.size();
        if (f.id == 0) pushRegs(_);
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
        if (f.id == 0) popRegs(_);
        _.ret();
    }

    void MachCodeGenerator::femms() {
        _.femms();
        _currentMode = CM_NORMAL;
    }

    void MachCodeGenerator::emitCall(IR::Call const &call) {
        if (_currentMode == CM_X87) femms();
    }


    void MachCodeGenerator::visit(const IR::Return *const expr) { //ok

        auto a = expr->atom;
        switch (_deriver.visitElement(expr->atom)) {

            case IR::VT_Int:
            case IR::VT_Ptr:
                _nextAssignment = RegOrMem(rax);
                break;
            case IR::VT_Double:
                _nextAssignment = RegOrMem(xmm0);
                break;
            default:
                throw std::invalid_argument("Invalid return expression type!");
        }
        expr->atom->visit(this);
    }


    void MachCodeGenerator::visit(const IR::Assignment *const expr) {
        if (typeOf(expr->var->id) != IR::VT_Unit)
        {
            _nextAssignedVreg = expr->var->id;
        _nextAssignment = locate(expr->var->id);
        }
        expr->value->visit(this);
    }

    void MachCodeGenerator::visit(const IR::FunctionRecord *const expr) {
        _currentFunction = expr;
        _.bind(functions[expr]->label);
        prologue(*expr);
        auto blocks = blocksPostOrder(expr->entry);
        for (auto b : blocks) visit(b);

        epilogue(*expr);
    }


    void MachCodeGenerator::visit(const IR::Block *const expr) {
        initBlock(expr);
        _.bind(blocks[expr].label);
        for (auto st: expr->contents)
            st->visit(this);
        if (!expr->isLastBlock()) expr->getTransition()->visit(this);
    }


    //assumes it is not an argument stored into stack
    X86Reg const *MachCodeGenerator::regOfVar(IR::VarId id) const {
        FOR_INT_DOUBLE(typeOf(id),
                return &gpFromVirt(virtRegOf(id));,
                return &xmmFromVirt(virtRegOf(id));)
    }

//    void MachCodeGenerator::genAssign(X86Mem const &dest, IR::Atom const *const atom) {
//        switch (atom->getType()) {
//            case IR::IrElement::IT_Variable: {
//                const IR::Variable *const var = atom->asVariable();
//                auto const *const memArg = functions[_currentFunction]->isMemArg(var->id);
//                if (memArg) {
//                    _.mov(tempGpRegs[0], ptr(rbp, memArg->ebpOffset, 1));
//                    _.mov(dest, tempGpRegs[0]);
//                }
//                else { //not a memory argument
//                    FOR_INT_DOUBLE(
//                            typeOf(var->id),
//                            {
//                                _.mov(dest, gpFromVirt(virtRegOf(var->id)));
//                            },
//                            {
//                                _.movq(dest, xmmFromVirt(virtRegOf(var->id)));
//                            })
//                }
//                break;
//            };
//            case IR::IrElement::IT_Int:
//                _.mov(dest, atom->asInt()->value);
//                break;
//            case IR::IrElement::IT_Double:
//                _.mov(dest, D2I(atom->asDouble()->value));
//                break;
//            case IR::IrElement::IT_Ptr: {
//                IR::Ptr const *const ptr = atom->asPtr();
//                _.mov(dest, contents(ptr));
//                break;
//            }
//            case IR::IrElement::IT_ReadRef:
//                throw std::invalid_argument("readref not supported yet");
//            default:
//                throw std::invalid_argument("attempt to move non-atom into memory");
//        };
//    }

//    void MachCodeGenerator::genAssign(X86Reg const &dest, IR::Atom const *const atom) {
//        FOR_ATOM(atom,
//                {
//                    FOR_REGTYPES(&dest,
//                            _.mov(gpReg, aInt->value);, {
//                        _.mov(tempGpRegs[0], aInt->value);
//                        _.movq(xmmReg, tempGpRegs[0]);
//                    });
//                },
//                {
//                    FOR_REGTYPES(&dest,
//                            _.mov(gpReg, D2I(aDouble->value));, {
//                        _.mov(tempGpRegs[0], D2I(aDouble->value));
//                        _.movq(xmmReg, tempGpRegs[0]);
//                    });
//                },
//                {
//                    FOR_REGTYPES(&dest, {
//                            if (aPtr->isPooledString) _.mov(gpReg, imm_ptr((void *) _ir.pool[aPtr->value].c_str()));
//                            else _.mov(gpReg, aPtr->value);
//                    }, {
//                            if (aPtr->isPooledString) {
//                                _.mov(tempGpRegs[0], imm_ptr((void *) _ir.pool[aPtr->value].c_str()));
//                                _.movq(xmmReg, tempGpRegs[0]);
//                            }
//                            else {
//                                _.mov(tempGpRegs[0], aPtr->value);
//                                _.movq(xmmReg, tempGpRegs[0]);
//                            }
//                    });
//                },
//                {
//                    auto const *const memArg = functions[_currentFunction]->isMemArg(aVar->id);
//                    if (memArg) {
//                        FOR_REGTYPES(&dest,
//                                {
//                                        _.mov(gpReg, ptr(rbp, memArg->ebpOffset, 1));
//                                },
//                                {
//                                        _.movq(xmmReg, ptr(rbp, memArg->ebpOffset, 1));
//                                });
//                    }
//                    else { //not a memory argument
//                        FOR_REGTYPES(&dest,
//                                FOR_INT_DOUBLE(
//                                        typeOf(aVar->id),
//                                        {
//                                                _.mov(gpReg, gpFromVirt(virtRegOf(aVar)));
//                                        },
//                                        {
//                                                _.movq(gpReg, xmmFromVirt(virtRegOf(aVar)));
//                                        }),
//                                FOR_INT_DOUBLE(
//                                        typeOf(aVar->id),
//                                        {
//                                                _.movq(xmmReg, gpFromVirt(virtRegOf(aVar)));
//                                        },
//                                        {
//                                                _.movq(xmmReg, xmmFromVirt(virtRegOf(aVar)));
//                                        })
//                        )
//                    }
//                },
//                {
//                    (void) aReadRef;
//                    throw std::invalid_argument("readref not supported yet");
//                }
//        )
//    }

//    void MachCodeGenerator::genAssign(RegOrMem const &dest, IR::Atom const *const atom) {
//        RegOrMemMatch(dest, genAssign(mem, atom);, genAssign(gpReg, atom);, genAssign(xmmReg, atom););
//    }


    void MachCodeGenerator::visit(IR::Variable const *const var) {
        auto rm = locate(var);
        RegOrMemMatchNoRedef(rm, {
            RegOrMemMatchNoRedef(_nextAssignment,
                    _.mov(tempGpRegs[0], rm.mem);
            _.mov(_nextAssignment.mem, tempGpRegs[0]);,
            _.mov(_nextAssignment.gp, rm.mem);,
            _.movq(_nextAssignment.xmm, rm.mem);
            );
        }, {
            RegOrMemMatchNoRedef(_nextAssignment,
                    _.mov(_nextAssignment.mem, rm.gp),
                    _.mov(_nextAssignment.mem, rm.gp),
            _.movq(_nextAssignment.xmm, rm.gp))
        }, {
            RegOrMemMatch(_nextAssignment,
                    _.movq(_nextAssignment.mem, rm.xmm),
                    _.movq(_nextAssignment.gp, rm.xmm),
            _.movq(_nextAssignment.xmm,rm.xmm))
        });
    }

    void MachCodeGenerator::visit(IR::Phi const *const expr) {
        throw std::bad_function_call();
    }

    void MachCodeGenerator::visit(IR::Int const *const expr) {
        RegOrMemMatch(_nextAssignment,
                _.mov(mem, expr->value),
                _.mov(gpReg, expr->value),
                _.mov(tempGpRegs[0], expr->value);
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
        const uint64_t value = (expr->isPooledString) ? (uint64_t) (&_runtime.stringPool[expr->value]) : expr->value;
        RegOrMemMatch(_nextAssignment,
                _.mov(mem, value),
                _.mov(gpReg, value),
                _.mov(tempGpRegs[0], value);
                _.movq(xmmReg, tempGpRegs[0]));
    }

    void MachCodeGenerator::visit(IR::ReadRef const *const expr) {

    }

    void MachCodeGenerator::visit(const IR::BinOp *const expr) {

        switch (expr->type) {

            case IR::BinOp::BO_ADD:
            case IR::BinOp::BO_FADD:
            case IR::BinOp::BO_MUL:
            case IR::BinOp::BO_FMUL:
            case IR::BinOp::BO_EQ:
            case IR::BinOp::BO_NEQ:
            case IR::BinOp::BO_OR:
            case IR::BinOp::BO_AND:
            case IR::BinOp::BO_LOR:
            case IR::BinOp::BO_LAND:
            case IR::BinOp::BO_XOR:
                if (expr->right->isVariable() && expr->right->asVariable()->id == _nextAssignedVreg) {
                    genReducedBinOp(expr->type, _nextAssignment, expr->right, _);
                    return;
                }

            case IR::BinOp::BO_DIV:
            case IR::BinOp::BO_FDIV:
            case IR::BinOp::BO_SUB:
            case IR::BinOp::BO_FSUB:
            case IR::BinOp::BO_MOD:
            case IR::BinOp::BO_LT:
            case IR::BinOp::BO_FLT:
            case IR::BinOp::BO_LE:
            case IR::BinOp::BO_FLE:
                if (expr->left->isVariable() && expr->left->asVariable()->id == _nextAssignedVreg) {
                    {
                        genReducedBinOp(expr->type, _nextAssignment, expr->right, _);
                        return;
                    }
                }
                genBinOp(expr->type, _nextAssignment, expr->left, expr->right, _);
                return;
            default:
                throw std::invalid_argument("native codegen for this operation is unsupported");
        };
    }

    void MachCodeGenerator::visit(const IR::UnOp *const unop) {
        genUnOp(unop->type, _nextAssignment, unop->operand, _);
        return;
//todo this should be done in  a generic way
//        switch (unop->type) {
//            case IR::UnOp::UO_CAST_D2I: {
//                RegOrMemMatch(_nextAssignment,
//                        {
//                            genAssign(tempXmmRegs[0], unop->operand);
//                            _.cvtsd2si(tempGpRegs[0], tempXmmRegs[0]);
//                            _.mov(mem, tempGpRegs[0]);
//                        },
//                        {
//                            genAssign(tempXmmRegs[0], unop->operand);
//                            _.cvtsd2si(gpReg, tempXmmRegs[0]);
//                        },
//                        {
//                            genAssign(tempXmmRegs[0], unop->operand);
//                            _.cvtsd2si(tempGpRegs[0], tempXmmRegs[0]);
//                            _.movq(xmmReg, tempGpRegs[0]);
//                        }
//                );
//                break;
//            };
//            case IR::UnOp::UO_NEG: {
//                RegOrMemMatch(_nextAssignment,
//                        {
//                            genAssign(tempGpRegs[0], unop->operand);
//                            _.neg(tempGpRegs[0]);
//                            _.mov(mem, tempGpRegs[0]);
//                        },
//                        {
//                            genAssign(gpReg, unop->operand);
//                            _.neg(gpReg);
//                        },
//                        {
//                            genAssign(tempGpRegs[0], unop->operand);
//                            _.neg(tempGpRegs[0]);
//                            _.movq(xmmReg, tempGpRegs[0]);
//                        }
//                );
//                break;
//            };
//            case IR::UnOp::UO_NOT: {
//                RegOrMemMatch(_nextAssignment,
//                        {
//                            genAssign(tempGpRegs[0], unop->operand);
//                            _.test(tempGpRegs[0], tempGpRegs[0]);
//                            _.sete(tempOneByteRegs[0]);
//                            _.movzx(tempGpRegs[0], tempOneByteRegs[0]);
//                            _.mov(mem, tempGpRegs[0]);
//                        },
//                        {
//                            genAssign(gpReg, unop->operand);
//                            _.test(gpReg, gpReg);
//                            _.sete(tempOneByteRegs[0]);
//                            _.movzx(gpReg, tempOneByteRegs[0]);
//                        },
//                        {
//                            genAssign(tempGpRegs[0], unop->operand);
//                            _.test(tempGpRegs[0], tempGpRegs[0]);
//                            _.sete(tempOneByteRegs[0]);
//                            _.movzx(tempGpRegs[0], tempOneByteRegs[0]);
//                            _.movq(xmmReg, tempGpRegs[0]);
//                        }
//                );
//                break;
//            }
//            default:
//                throw std::invalid_argument("Invalid unary operation type");
//        }
    }

    void MachCodeGenerator::visit(const IR::Call *const expr) {
        IR::FunctionRecord const* const calledFunction = _ir.functions[expr->funId];
        Function const& descr = * functions[calledFunction];
        std::stack<X86Reg const*> savedRegs;

        //first save all registers we need

        //then insert arguments
        for (size_t i = 0; i < descr.arguments.size(); i++) {
            switch( descr.arguments[i].class_) {
                case AC_INTEGER:
                    _nextAssignment = RegOrMem(descr.arguments[i].gpReg);
                    break;
                case AC_SSE:
                    _nextAssignment = RegOrMem(descr.arguments[i].xmmReg);
                    break;
                case AC_MEM:
                    _nextAssignment = RegOrMem(X86Mem(ptr(rbp, descr.arguments[i].getRbpOffset(),8)));
                    break;
            }
            expr->params[i]->visit(this);
        }
        //todo reference parameters!

        //emit call
        _.call(functions[_ir.functions[expr->funId]]->label);
        //pop registers
    }

    static void print_int(int64_t i) {
        printf("%ld", i);
    }

    static void print_double(double d) {
        printf("%lf", d);
    }

    static void print_str(char const *s) {
        printf("%s", s);
    }

    void MachCodeGenerator::visit(const IR::Print *const expr) {

        FOR_ATOM(expr->atom,
                {
                    _.mov(integerArgumentsRegs[0], contents(aInt));
                    _.call(Ptr(&print_int));
                },
                {//double
                    const double cnt = contents(aDouble);
                    _.mov(tempGpRegs[0], D2I(cnt));
                    _.movq(sseArgumentsRegs[0], tempGpRegs[0]);
                    _.call(Ptr(&print_double));
                },
                {//ptr
                    _.mov(integerArgumentsRegs[0], contents(aPtr));
                    _.call(Ptr(&print_str));
                },
                {
                    const RegOrMem rm = locate(aVar);
                    RegOrMemMatch(rm, {
                                    if (typeOf(aVar->id) == IR::VT_Double)
                                    {
                                        _.movq(sseArgumentsRegs[0], mem);
                                        _.call(Ptr(&print_double));
                                    }
                                    else if (typeOf(aVar->id) == IR::VT_Ptr)
                                    {
                                        _.mov(integerArgumentsRegs[0], mem);
                                        _.call(Ptr(&print_str));
                                    } else {
                                        _.mov(integerArgumentsRegs[0], mem);
                                        _.call(Ptr(&print_int));
                                    }
                            },
                            {
                                    _.mov(integerArgumentsRegs[0], gpReg);
                                    _.call(Ptr(&print_int));
                            },
                            {
                                    _.movq(sseArgumentsRegs[0], xmmReg);
                                    _.call(Ptr(&print_double));
                            }
                    );
                }, {
            //readref not supported
        }
        );


    }


    void MachCodeGenerator::visit(const IR::JumpAlways *const expr) {
        initBlock(expr->destination);
        _.jmp(blocks[expr->destination].label);
    }

    void MachCodeGenerator::visit(const IR::JumpCond *const expr) {
        initBlock(expr->yes);
        initBlock(expr->no);
        FOR_ATOM(expr->condition,
                _.jmp(blocks[(contents(aInt)) ? expr->yes : expr->no].label);,
                _.jmp(blocks[(contents(aDouble)) ? expr->yes : expr->no].label);,
                _.jmp(blocks[(contents(aPtr)) ? expr->yes : expr->no].label);,
                {
                    RegOrMemMatch(locate(aVar),
                            _.mov(rax, mem);
                    _.test(rax, rax);,
                    _.test(gpReg, gpReg);,
                    _.movq(rax, xmmReg);
                    _.test(rax, rax);
                    );
                }, {
            throw std::invalid_argument("readref not supported yet");
        };
        );
        _.jz(blocks[expr->no].label);
        _.jmp(blocks[expr->yes].label);
    }

    void MachCodeGenerator::visit(const IR::WriteRef *const expr) {

    }


    bool CodeGenErrorHandler::handleError(Error code, const char *message) {
        _debug << message << std::endl;
        throw std::invalid_argument(message);
        return true;
    }

    void *MachCodeGenerator::translate() {
        _debug << "\n-------------------------------\n   Code generation has started \n-------------------------------\n";

        StringLogger logger;
        // CodeGenErrorHandler handler(_debug);
        _.setLogger(&logger);
        // _.setErrorHandler(&handler);
        for (auto f: _ir.functions) initFunction(f);
        for (auto f: _ir.functions) visit(f);

        void *result = _.make();
        _debug << "Logged:\n" << logger.getString() << std::endl;
        _debug << ErrorUtil::asString(_.getError()) << std::endl;

        if (result) return result;
        return NULL;
    }

    X86XmmReg const &MachCodeGenerator::xmmFromVirt(uint64_t vreg) const {
        return allocableXmmRegs[functions.at(_currentFunction)->regsXmm[vreg]];
    }

    X86GpReg const &MachCodeGenerator::gpFromVirt(uint64_t vreg) const {
        return allocableRegs[functions.at(_currentFunction)->regsGp[vreg]];
    }

    RegOrMem MachCodeGenerator::locate(IR::VarId id) const {
        Function::Argument const* const arg = functions.at(_currentFunction)->argumentForVar(id);
        if (arg && arg->class_ == AC_MEM)
            return RegOrMem(X86Mem(rbp,arg->getRbpOffset()));
        else {
            auto vreg = virtRegOf(id);
            FOR_INT_DOUBLE(typeOf(id),
                    return RegOrMem(gpFromVirt(vreg));,
                    return RegOrMem(xmmFromVirt(vreg));)
        }
    }


    bool MachCodeGenerator::isRegAliveAfter(const IR::Statement* const statement, X86GpReg const& reg) {
        size_t* regs = functions.at(_currentFunction)->regsGp;
        for( size_t vreg = 0; vreg < ALLOCABLE_REGS_COUNT; vreg++)
            if (allocableRegs[regs[vreg]].getId() == reg.getId()) {
                auto regIt = allocations().vregToVarId.find(vreg);
                if (regIt == allocations().vregToVarId.end()) return false;
                IR::VarId varid = (*regIt).second;
                return _liveInfo->data.at(_currentFunction)->varIntervals.at(varid).contains(statement->num+1);
        }
        throw std::invalid_argument("This register is not allocable");
    }

    bool MachCodeGenerator::isRegAliveAfter(const IR::Statement *const statement, X86XmmReg const &reg) {
        size_t* regs = functions.at(_currentFunction)->regsXmm;
        for( size_t vreg = 0; vreg < ALLOCABLE_REGS_COUNT; vreg++)
            if (allocableRegs[regs[vreg]].getId() == reg.getId()) {
                auto regIt = allocations().vregToVarId.find(vreg);
                if (regIt == allocations().vregToVarId.end()) return false;
                IR::VarId varid = (*regIt).second;
                return _liveInfo->data.at(_currentFunction)->varIntervals.at(varid).contains(statement->num+1);
            }
        throw std::invalid_argument("This register is not allocable");
    }

    MachCodeGenerator::Info::Info() {
        for (size_t i = 0; i < INTEGER_ARGUMENTS_REGS_LENGTH; i++)
            nextIntArg.push_back(std::find(allocableRegs, std::end(allocableRegs), integerArgumentsRegs[i]) - allocableRegs);
        for (size_t i = 0; i < SSE_ARGUMENTS_REGS_LENGTH; i++)
            nextSseArg.push_back(std::find(allocableXmmRegs, std::end(allocableXmmRegs), sseArgumentsRegs[i]) - allocableXmmRegs);
    }

    void MachCodeGenerator::Function::setupRegisterMapping() {
        bool assignedGp[ALLOCABLE_REGS_COUNT] = {false};
        bool assignedXmm[ALLOCABLE_REGS_COUNT] = {false};
        size_t intArgs = 0, sseArgs = 0;
        int32_t memArgs = 0;

        std::set<size_t> freeGpRegs;
        std::set<size_t> freeXmmRegs;

        for (size_t i = 0; i < INTEGER_ARGUMENTS_REGS_LENGTH; i++) freeGpRegs.insert(i);
        for (size_t i = 0; i < SSE_ARGUMENTS_REGS_LENGTH; i++) freeXmmRegs.insert(i);

        for (Argument const &arg : arguments) {
            switch (arg.class_) {
                case AC_INTEGER:
                {
                    auto regIdx= std::find(allocableRegs, std::end(allocableRegs), arg.gpReg) - allocableRegs;
                    regsGp[arg.var] = regIdx;
                    assignedGp[arg.var] = true;
                    freeGpRegs.erase(regIdx);
                    break;
                }
                case AC_SSE:
                {
                    auto regIdx= std::find(allocableXmmRegs, std::end(allocableXmmRegs), arg.xmmReg) - allocableXmmRegs;
                    regsXmm[arg.var] = regIdx;
                    assignedXmm[arg.var] = true;
                    freeXmmRegs.erase(regIdx);
                    break;
                }
                    break;
                case AC_MEM:
                    break;
            };
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

    void MachCodeGenerator::genAssign(RegOrMem const &from, RegOrMem const &to) {
        RegOrMemMatchNoRedef(from,
                RegOrMemMatchNoRedef(to,
                        _.mov(rax, from.mem);
                _.mov(to.mem, rax);,
                _.mov(to.gp, from.mem),
                _.movq(to.xmm, from.mem)),

                RegOrMemMatchNoRedef(to, _.mov(to.mem, from.gp);, _.mov(to.gp, from.gp), _.movq(to.xmm, from.gp)),

                RegOrMemMatchNoRedef(to, _.movq(to.mem, from.xmm);, _.movq(to.gp, from.xmm), _.movq(to.xmm, from.xmm);)
        )
    }
}

//    MachCodeGenerator::FunctionDescriptor::FunctionDescriptor(IR::SimpleIr const &ir, IR::RegAllocInfo const &regAllocInfo, IR::FunctionRecord const &f, asmjit::Label const &label)
//            : label(label), function(f) {
//
//        std::vector<size_t> nextIntArg;
//        std::vector<size_t> nextSseArg;
//        for (size_t i = 0; i < INTEGER_ARGUMENTS_REGS_LENGTH; i++)
//            nextIntArg.push_back(std::find(allocableRegs, std::end(allocableRegs), integerArgumentsRegs[i]) - allocableRegs);
//        for (size_t i = 0; i < SSE_ARGUMENTS_REGS_LENGTH; i++)
//            nextSseArg.push_back(std::find(allocableXmmRegs, std::end(allocableXmmRegs), sseArgumentsRegs[i]) - allocableXmmRegs);
//
//
//        std::set<size_t> freeGpRegs;
//        std::set<size_t> freeXmmRegs;
//
//        for (size_t i = 0; i < INTEGER_ARGUMENTS_REGS_LENGTH; i++) freeGpRegs.insert(i);
//        for (size_t i = 0; i < SSE_ARGUMENTS_REGS_LENGTH; i++) freeXmmRegs.insert(i);
//
//        bool assignedGp[ALLOCABLE_REGS_COUNT] = {false};
//        bool assignedXmm[ALLOCABLE_REGS_COUNT] = {false};
//
//        for (size_t i = 0; i < f.arguments(); ++i) {
//            auto argId = f.argument(i);
//            uint64_t vreg = regAllocInfo.regAlloc.at(argId);
//            switch (argumentClassForType(ir.varMeta[argId].type)) {
//                case AC_INTEGER:
//                    if (!nextIntArg.empty()) {
//                        auto reg = nextIntArg.back();
//                        nextIntArg.pop_back();
//                        regsGp[vreg] = reg;
//                        assignedGp[vreg] = true;
//                        freeGpRegs.erase(vreg);
//                    }
//                    break;
//                case AC_SSE:
//                    if (!nextSseArg.empty()) {
//                        auto reg = nextSseArg.back();
//                        nextSseArg.pop_back();
//                        regsXmm[vreg] = reg;
//                        assignedXmm[vreg] = true;
//                        freeXmmRegs.erase(vreg);
//                    }
//                    break;
//                default:
//                    throw std::invalid_argument("Unknown argument class");
//            }
//        }
//
//        auto freeGpIt = freeGpRegs.cbegin();
//        auto freeXmmIt = freeXmmRegs.cbegin();
//        for (size_t i = 0; i < ALLOCABLE_REGS_COUNT; i++) {
//            if (!assignedGp[i]) {
//                regsGp[i] = *freeGpIt;
//                freeGpIt++;
//            }
//            if (!assignedXmm[i]) {
//                regsXmm[i] = *freeXmmIt;
//                freeXmmIt++;
//            }
//        }
//    }
