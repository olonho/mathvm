#include "machcode_generator.h"
#include <queue>
#include <algorithm>


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


    CodeGenerator::CodeGenerator(IR::SimpleIr const &ir, MvmRuntime &runtime,std::ostream &debug)
            :
            _ir(ir),
            _runtime(runtime),
            _(& _runtime.jitRuntime),
            _liveInfo(IR::LiveAnalyzer(debug).start(ir)),
            regAllocInfo(IR::regAlloc(ir, *_liveInfo, ALLOCABLE_REGS_COUNT, debug)),
            _debug(debug),
            _irPrinter(std::cerr),
            _deriver(_ir.varMeta, _ir.functions, _debug) {
        runtime.stringPool = ir.pool;
        _functions.resize(ir.functions.size());
    }

    CodeGenerator::Function::Function(CodeGenerator &generator, IR::FunctionRecord const &f, asmjit::Label const &label)
            : label(label), function(f), generator(generator), orderedBlocks(generator._liveInfo->data.at(f.id)->orderedBlocks) {
        setupArguments();
        setupRegisterMapping();
    }


    void CodeGenerator::prologue(IR::FunctionRecord const &f) {
        const size_t locals = f.memoryCells.size();
        if (locals > 0 || _functions[f.id]->hasMemArgs()) {
            _.push(rbp);
            _.mov(rbp, rsp);
        }
        if (locals > 0) _.sub(rsp, locals);
    }

    void CodeGenerator::epilogue(IR::FunctionRecord const &f) {
        const size_t locals = f.memoryCells.size();

        if (locals > 0 || _functions[f.id]->hasMemArgs()) {
            _.mov(rsp, rbp);
            _.pop(rbp);
        }
        _.ret();
    }

    int64_t CodeGenerator::contents(IR::Ptr const *ptr) const {
        return (ptr->isPooledString) ? (uint64_t) (_runtime.stringPool[ptr->value].c_str()) : ptr->value;
    }

    void CodeGenerator::visit(const IR::Return *const expr) {
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


    void CodeGenerator::visit(const IR::Assignment *const expr) {
        if (typeOf(expr->var->id) != IR::VT_Unit) {
            _nextAssignedVreg = virtRegOf(expr->var->id);
            _nextAssignment = locate(expr->var->id);
        }
        expr->value->visit(this);
    }

    void CodeGenerator::visit(const IR::FunctionRecord *const expr) {
        _currentFunction = expr;
        prologue(*expr);
        auto& blocks = _functions[expr->id]->orderedBlocks;
        //std::reverse(blocks.begin(), blocks.end());
        for (auto b : blocks) initBlock(b);
        for (auto b : blocks) visit(b);

    }


    void CodeGenerator::visit(const IR::Block *const expr) {
        _debug << "visiting block " << expr->name << "\n";

        if (_currentFunction->entry == expr)
            _.bind(_functions[_currentFunction->id]->label);

        _.bind(_blocks[expr].label);
        for (auto st: expr->contents)
            st->visit(this);
        if (!expr->isLastBlock()) expr->getTransition()->visit(this);
        else epilogue(*_currentFunction);
    }

    void CodeGenerator::visit(IR::Variable const *const var) {
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
                    _.mov(_nextAssignment.gp, rm.gp),
                    _.movq(_nextAssignment.xmm, rm.gp))
        }, {
            RegOrMemMatchNoRedef(_nextAssignment,
                    _.movq(_nextAssignment.mem, rm.xmm),
                    _.movq(_nextAssignment.gp, rm.xmm),
                    _.movq(_nextAssignment.xmm, rm.xmm))
        });
    }

    void CodeGenerator::visit(IR::Phi const *const expr) {
        throw std::bad_function_call();
    }

    void CodeGenerator::visit(IR::Int const *const expr) {
        RegOrMemMatch(_nextAssignment,
                _.mov(mem, expr->value),
                _.mov(gpReg, expr->value),
                _.mov(tempGpRegs[0], expr->value);
                _.movq(xmmReg, tempGpRegs[0]));
    }

    void CodeGenerator::visit(IR::Double const *const expr) {
        const int64_t value = D2I(expr->value);
        RegOrMemMatch(_nextAssignment,
                _.mov(mem, value),
                _.mov(gpReg, value),
                _.mov(tempGpRegs[0], value);
                _.movq(xmmReg, tempGpRegs[0]))
    }

    void CodeGenerator::visit(IR::Ptr const *const expr) {
        const int64_t value = contents(expr);
        RegOrMemMatch(_nextAssignment,
                _.mov(mem, value),
                _.mov(gpReg, value),
                _.mov(tempGpRegs[0], value);
                _.movq(xmmReg, tempGpRegs[0]));
    }

    void CodeGenerator::visit(IR::ReadRef const *const expr) {

    }

    void CodeGenerator::visit(const IR::BinOp *const expr) {

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
                if (expr->right->isVariable() && virtRegOf(expr->right->asVariable()->id) == _nextAssignedVreg) {
                    genReducedBinOp(expr->type, _nextAssignment, expr->left, _);
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
                if (expr->left->isVariable() && virtRegOf(expr->left->asVariable()->id) == _nextAssignedVreg) {
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

    void CodeGenerator::visit(const IR::UnOp *const unop) {
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

    void CodeGenerator::visit(const IR::Call *const expr) {
//        IR::FunctionRecord const *const calledFunction = _ir.functions[expr->funId];
        Function const &descr = *_functions[expr->funId];
        std::stack<X86Reg const *> savedRegs;
        RegOrMem writeResultInto = _nextAssignment;
        //first save all registers we need

        //then insert arguments
        for (size_t i = 0; i < descr.arguments.size(); i++) {
            switch (descr.arguments[i].class_) {
                case AC_INTEGER:
                    _nextAssignment = RegOrMem(descr.arguments[i].gpReg);
                    break;
                case AC_SSE:
                    _nextAssignment = RegOrMem(descr.arguments[i].xmmReg);
                    break;
                case AC_MEM:
                    _nextAssignment = RegOrMem(X86Mem(ptr(rbp, descr.arguments[i].getRbpOffset(), 8)));
                    break;
            }
            expr->params[i]->visit(this);
        }
        //todo reference parameters!

        //emit call
        _.call(descr.label);
        //pop registers


        if (_ir.functions[expr->funId]->returnType == IR::VT_Unit) {}
        else if (_ir.functions[expr->funId]->returnType == IR::VT_Double)
            genAssign(RegOrMem(xmm0), writeResultInto);
        else
            genAssign(RegOrMem(rax), writeResultInto);

    }


    void CodeGenerator::visit(const IR::Print *const expr) {

        FOR_ATOM(expr->atom,
                {
                    _.mov(integerArgumentsRegs[0], contents(aInt));
                    _.call(Ptr(&MvmRuntime::print_int));
                },
                {//double
                    const double cnt = contents(aDouble);
                    _.mov(tempGpRegs[0], D2I(cnt));
                    _.movq(sseArgumentsRegs[0], tempGpRegs[0]);
                    _.call(Ptr(&MvmRuntime::print_double));
                },
                {//ptr
                    _.mov(integerArgumentsRegs[0], contents(aPtr));
                    _.call(Ptr(&MvmRuntime::print_str));
                },
                {
                    const RegOrMem rm = locate(aVar);
                    RegOrMemMatch(rm, {
                                    if (typeOf(aVar->id) == IR::VT_Double)
                                    {
                                        _.movq(sseArgumentsRegs[0], mem);
                                        _.call(Ptr(&MvmRuntime::print_double));
                                    }
                                    else if (typeOf(aVar->id) == IR::VT_Ptr)
                                    {
                                        _.mov(integerArgumentsRegs[0], mem);
                                        _.call(Ptr(&MvmRuntime::print_str));
                                    } else {
                                        _.mov(integerArgumentsRegs[0], mem);
                                        _.call(Ptr(&MvmRuntime::print_int));
                                    }
                            },
                            {
                                    _.mov(integerArgumentsRegs[0], gpReg);
                                    _.call(Ptr(&MvmRuntime::print_int));
                            },
                            {
                                    _.movq(sseArgumentsRegs[0], xmmReg);
                                    _.call(Ptr(&MvmRuntime::print_double));
                            }
                    );
                }, {
            (void) aReadRef;
            //readref not supported
        }
        );


    }


    void CodeGenerator::visit(const IR::JumpAlways *const expr) {
        _.jmp(_blocks[expr->destination].label);
    }

    void CodeGenerator::visit(const IR::JumpCond *const expr) {
        FOR_ATOM(expr->condition,
                _.jmp(_blocks[(contents(aInt)) ? expr->yes : expr->no].label);,
                _.jmp(_blocks[(contents(aDouble)) ? expr->yes : expr->no].label);,
                _.jmp(_blocks[(contents(aPtr)) ? expr->yes : expr->no].label);,
                {
                    RegOrMemMatch(locate(aVar),
                            _.mov(rax, mem);
                    _.test(rax, rax);,
                    _.test(gpReg, gpReg);,
                    _.movq(rax, xmmReg);
                    _.test(rax, rax);
                    );
                    _.jz(_blocks[expr->no].label);
                    _.jmp(_blocks[expr->yes].label);
                }, {
            (void) aReadRef;
            throw std::invalid_argument("readref not supported yet");
        };
        );
    }

    void CodeGenerator::visit(const IR::WriteRef *const expr) {

    }


    bool CodeGenErrorHandler::handleError(Error code, const char *message) {
        _debug << message << std::endl;
        return true;
    }

    void *CodeGenerator::translate() {
        _debug << "\n-------------------------------\n   Code generation has started \n-------------------------------\n";


        StringLogger logger;
        _.setErrorHandler(new CodeGenErrorHandler(_debug));
        _.setLogger(&logger);

        for (auto f: _ir.functions) initFunction(f);
        for (auto f: _ir.functions) visit(f);

        void *result = _.make();
        _debug << "Logged:\n" << logger.getString() << std::endl;

        if (result) return result;
        return NULL;
    }

    X86XmmReg const &CodeGenerator::xmmFromVirt(uint64_t vreg) const {
        return allocableXmmRegs[_functions.at(_currentFunction->id)->regsXmm[vreg]];
    }

    X86GpReg const &CodeGenerator::gpFromVirt(uint64_t vreg) const {
        return allocableRegs[_functions.at(_currentFunction->id)->regsGp[vreg]];
    }

    RegOrMem CodeGenerator::locate(IR::VarId id) const {
        Function::Argument const *const arg = _functions.at(_currentFunction->id)->argumentForVar(id);
        if (arg)
            switch (arg->class_) {
                case AC_INTEGER:
                    return RegOrMem(arg->gpReg);
                case AC_SSE:
                    return RegOrMem(arg->xmmReg);
                case AC_MEM:
                    return RegOrMem(X86Mem(rbp, arg->getRbpOffset()));
                default:
                    throw std::invalid_argument("Can't locate var");
            }
        else {
            auto vreg = virtRegOf(id);
            FOR_INT_DOUBLE(typeOf(id),
                    return RegOrMem(gpFromVirt(vreg));,
                    return RegOrMem(xmmFromVirt(vreg));)
        }

    }

    bool CodeGenerator::isRegAliveAfter(const IR::Statement *const statement, X86GpReg const &reg) {
        size_t *regs = _functions.at(_currentFunction->id)->regsGp;
        for (size_t vreg = 0; vreg < ALLOCABLE_REGS_COUNT; vreg++)
            if (allocableRegs[regs[vreg]].getId() == reg.getId()) {
                auto regIt = allocations().vregToVarId.find(vreg);
                if (regIt == allocations().vregToVarId.end()) return false;
                IR::VarId varid = (*regIt).second;
                return _liveInfo->data.at(_currentFunction->id)->varIntervals.at(varid).contains(statement->num + 1);
            }
        throw std::invalid_argument("This register is not allocable");
    }

    bool CodeGenerator::isRegAliveAfter(const IR::Statement *const statement, X86XmmReg const &reg) {
        size_t *regs = _functions.at(_currentFunction->id)->regsXmm;
        for (size_t vreg = 0; vreg < ALLOCABLE_REGS_COUNT; vreg++)
            if (allocableRegs[regs[vreg]].getId() == reg.getId()) {
                auto regIt = allocations().vregToVarId.find(vreg);
                if (regIt == allocations().vregToVarId.end()) return false;
                IR::VarId varid = (*regIt).second;
                return _liveInfo->data.at(_currentFunction->id)->varIntervals.at(varid).contains(statement->num + 1);
            }
        throw std::invalid_argument("This register is not allocable");
    }

    CodeGenerator::Info::Info() {
        for (size_t i = 0; i < INTEGER_ARGUMENTS_REGS_LENGTH; i++)
            nextIntArg.push_back(std::find(allocableRegs, std::end(allocableRegs), integerArgumentsRegs[i]) - allocableRegs);
        for (size_t i = 0; i < SSE_ARGUMENTS_REGS_LENGTH; i++)
            nextSseArg.push_back(std::find(allocableXmmRegs, std::end(allocableXmmRegs), sseArgumentsRegs[i]) - allocableXmmRegs);
    }



    void CodeGenerator::genAssign(RegOrMem const &from, RegOrMem const &to) {
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



    void CodeGenerator::Function::setupRegisterMapping() {
        bool assignedGp[ALLOCABLE_REGS_COUNT] = {false};
        bool assignedXmm[ALLOCABLE_REGS_COUNT] = {false};

        std::set<size_t> freeGpRegs;
        std::set<size_t> freeXmmRegs;

        for (size_t i = 0; i < ALLOCABLE_REGS_COUNT; i++) freeGpRegs.insert(i);
        for (size_t i = 0; i < ALLOCABLE_REGS_COUNT; i++) freeXmmRegs.insert(i);

        for (Argument const& arg : arguments) {
            auto vreg = generator.virtRegOf(function.id, arg.var);

            switch (arg.class_) {
                case AC_INTEGER: {
                    auto regIdx = std::find(allocableRegs, std::end(allocableRegs), arg.gpReg) - allocableRegs;
                    regsGp[vreg] = regIdx;
                    assignedGp[vreg] = true;
                    freeGpRegs.erase(regIdx);
                    break;
                }
                case AC_SSE: {
                    auto regIdx = std::find(allocableXmmRegs, std::end(allocableXmmRegs), arg.xmmReg) - allocableXmmRegs;
                    regsXmm[vreg] = regIdx;
                    assignedXmm[vreg] = true;
                    freeXmmRegs.erase(regIdx);
                    break;
                }
                default:
                    break;
            };
        }

        auto freeGpIt = freeGpRegs.cbegin();
        auto freeXmmIt = freeXmmRegs.cbegin();
        for (size_t vreg = 0; vreg < ALLOCABLE_REGS_COUNT; vreg++) {
            if (!assignedGp[vreg]) {
                regsGp[vreg] = *freeGpIt;
                freeGpIt++;
            }
            if (!assignedXmm[vreg]) {
                regsXmm[vreg] = *freeXmmIt;
                freeXmmIt++;
            }
        }
        generator._debug << "GP reg mappings for functions" << function.id << ":\n";
        for (size_t i = 0; i < ALLOCABLE_REGS_COUNT; i++) {
            generator._debug << i << "->" << regsGp[i] <<  "\n";
        }
        generator._debug << "Xmm reg mappings for function " << function.id << ":\n";
        for (size_t i = 0; i < ALLOCABLE_REGS_COUNT; i++) {
            generator._debug << i << "->" << regsXmm[i] <<  "\n";
        }
    }
    void CodeGenerator::Function::setupArguments() {
        size_t intArgs = 0, sseArgs = 0;
        uint32_t memArgs = 0;

        for (size_t i = 0; i < function.arguments(); ++i) {
            auto argId = function.argument(i);
            const IR::VarType type = generator.typeOf(argId);
            switch (CodeGenerator::argumentClassForType(type)) {
                case AC_INTEGER:
                    if (intArgs < INTEGER_ARGUMENTS_REGS_LENGTH)
                        arguments.push_back(Argument(i, argId, type, integerArgumentsRegs[intArgs]));
                    else
                        arguments.push_back(Argument(i, argId, type, memArgs++));
                    intArgs++;
                    break;
                case AC_SSE:
                    if (sseArgs < SSE_ARGUMENTS_REGS_LENGTH)
                        arguments.push_back(Argument(i, argId, type, sseArgumentsRegs[intArgs]));
                    else
                        arguments.push_back(Argument(i, argId, type, memArgs++));
                    sseArgs++;
                    break;
                case AC_MEM:
                    arguments.push_back(Argument(i, argId, type, memArgs++));
                    break;
            };
        }
    }
}

//    CodeGenerator::FunctionDescriptor::FunctionDescriptor(IR::SimpleIr const &ir, IR::RegAllocInfo const &regAllocInfo, IR::FunctionRecord const &f, asmjit::Label const &label)
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
