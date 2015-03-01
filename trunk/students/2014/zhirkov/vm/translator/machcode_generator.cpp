#include "machcode_generator.h"
#include "../exceptions.h"
#include <queue>
#include <algorithm>
#include <sstream>

#define FOR_REGTYPES(reg, actGP, actXMM) if ((reg)->isGp())\
{ X86GpReg const&  gpReg = *((X86GpReg const*) reg);  {actGP }}\
else { X86XmmReg const&  xmmReg = *((X86XmmReg const*) reg); {actXMM }}

#define FOR_INT_DOUBLE(type, actip, actd)  switch (type) {\
    case IR::VT_Int: case IR::VT_Ptr:{actip};    break; \
    case IR::VT_Double: {actd}; break;\
    default: throw BadIr("Expecting int, ptr or double");}

#define FOR_ATOM(atom, ifint, ifdouble, ifptr, ifvar, ifreadref) switch ((atom)->getType()) {\
    case IR::IrElement::IT_Variable: {const IR::Variable* const aVar = atom->asVariable(); ifvar; break;} \
    case IR::IrElement::IT_Int: {const IR::Int* const aInt = atom->asInt(); ifint;  break; } \
    case IR::IrElement::IT_Double: {const IR::Double* const aDouble = atom->asDouble(); ifdouble; break; }\
    case IR::IrElement::IT_Ptr: {const IR::Ptr* const aPtr = atom->asPtr();ifptr ; break;}\
    case IR::IrElement::IT_ReadRef: {const IR::ReadRef* const aReadRef = atom->asReadRef();ifreadref; break;};\
    default:throw BadIr("Not an atom");\
};


#define PREDEF_FUNCTIONS_SIZE 3
namespace mathvm {
    const X86GpReg callerSave[] = {
            rcx, rdx, rsi, rdi, r8, r9, r10, r11
    };
    const X86GpReg calleeSave[] = {
            rbx, r12, r13, r14, r15
    };

    const X86GpReg allocableRegs[] = {
            rdi, rsi, rdx, rcx, r12, r13, r14, r15, rbx, r8, r9, r11
    };
    const X86GpReg tempGpRegs[] = {
            rax
    };
    const X86GpReg tempOneByteRegs[]
            {
                    al};

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

    static void print_double(double d) {
        std::cout << d;
    }

    static void print_str(char const *s) {
        std::cout << s;
    }

    static void print_int(int64_t i) {
        std::cout << i;
    }

    CodeGenerator::CodeGenerator(IR::SimpleIr const &ir, MvmRuntime &runtime,
            std::ostream &debug) : _ir(ir),
                                   _varMeta(ir.varMeta),
                                   _runtime(runtime),
                                   _(&_runtime.jitRuntime),
                                   _debug(debug),
                                   _irPrinter(std::cerr),
                                   _deriver(_varMeta, _ir.functions,_debug),
                                   gpAccVar(_varMeta.size()),
                                   doubleAccVar(_varMeta.size() + 1),
                                   gpAccVreg(ALLOCABLE_REGS_COUNT),
                                   doubleAccVreg(ALLOCABLE_REGS_COUNT + 1),
                                   liveInfo(IR::LiveAnalyzer(true, debug).start(ir)),
                                   regAllocInfo(IR::regAlloc(ir, *liveInfo, ALLOCABLE_REGS_COUNT, debug)) {
        runtime.stringPool = ir.pool;
        _functions.resize(_ir.functions.size());

        IR::regAllocDump(regAllocInfo, _debug);
        for (auto f:_ir.functions)
            initFunction(f);
        setupPredefFunctions();
    }

    CodeGenerator::Function::Function(CodeGenerator &generator,
            IR::Function const &f,
            asmjit::
            Label const &label) : label(label),
                                  function(f), returnType(f.returnType), generator(generator),
                                  orderedBlocks(generator.liveInfo->data.at(f.id)->orderedBlocks),
                                  isPredef(false) {
        setupArguments();
        setupRegisterMapping();
    }

    CodeGenerator::Function::Function(CodeGenerator &generator, IR::Function const &f) : function(f),
                                     returnType(f.returnType),
                                     generator(generator),
                                     orderedBlocks((f.id != UINT16_MAX) ? generator.liveInfo->data.at(f.id)-> orderedBlocks : std::vector<IR::Block const *>()),
                                     isPredef(true) {
        setupRegisterMapping();
    }

    void CodeGenerator::prologue(CodeGenerator::Function const &f) {
        const size_t locals = f.function.memoryCells.size();
        for (auto const &reg:
                f.savedGpInPrologue)
            _.push(reg);

        if (hasStackFrame(f.function)) {
            _.push(rbp);
            _.mov(rbp, rsp);
        }
        //todo back up xmm registers used for arguments AFTER locals
        //

        if (locals > 0)
            _.sub(rsp, (locals + f.backedXmmArgsRegsRbpOffsets.size()) * 8) ;
        if (f.backedXmmArgsRegsRbpOffsets.size() >= 1) _.movq(ptr(rbp, -locals*8), xmm0);
        if (f.backedXmmArgsRegsRbpOffsets.size() >= 2) _.movq(ptr(rbp, -locals*8 - 8), xmm1);
    }

    void CodeGenerator::epilogue(CodeGenerator::Function const &f) {
        if (hasStackFrame(f.function)) {
            _.mov(rsp, rbp);
            _.pop(rbp);
        }

        //restore registers
        for (auto it = f.savedGpInPrologue.rbegin();
             it != f.savedGpInPrologue.rend(); it++)
            if (isCalleeSave(*it))
                _.pop(*it);

        _.ret();

    }

    bool CodeGenerator::hasStackFrame(IR::Function const &f) const {
        const size_t locals = f.memoryCells.size();
        return locals > 0 || _functions[f.id]->hasMemArgs()
                || hasCalls(f) || f.id == 0 || _functions[f.id]->backedXmmArgsRegsRbpOffsets.size() >= 1;
    }

    int64_t CodeGenerator::contents(IR::Ptr const *ptr) const {
        return (ptr->isPooledString) ? (uint64_t) (_runtime.
                stringPool[ptr->
                value].c_str()) :
                ptr->value;
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
                throw BadIr("Invalid return expression type!");
        }
        expr->atom->visit(this);
        epilogue(*_functions[_currentFunction->id]);
    }


    void CodeGenerator::visit(const IR::Assignment *const expr) {
        if (typeOf(expr->var->id) != IR::VT_Unit) {
            _nextAssignedVreg = virtRegOf(expr->var->id);
            _nextAssignment = locate(expr->var->id);
        }
        expr->value->visit(this);
    }

    void CodeGenerator::visit(const IR::Function *const expr) {
        if (expr->isNative())
            return;
        _currentFunction = expr;
        auto &blocks = _functions[expr->id]->orderedBlocks;

        _.bind(_functions[expr->id]->label);
        if (_.getLogger()) _.getLogger()->logFormat(0, "; Function id %d name %s\n", expr->id, expr->name.c_str());
        for (auto b:blocks)
            initBlock(b);

        prologue(*_functions[expr->id]);
        for (auto b:blocks)
            visit(b);

    }


    void CodeGenerator::visit(const IR::Block *const expr) {
//        _debug << "visiting block " << expr->name << "\n";
        _currentBlock = expr;

        if (_.getLogger()) _.getLogger()->logFormat(0, "; block %s\n", expr->name.c_str());

        _.bind(_blocks[expr].label);
        for (auto st:expr->contents) {
            _currentStatement = st;

            std::stringstream str; str<< *st;

            if (_.getLogger()) _.getLogger()->logFormat(0, "\n;> %s\n", str.str().c_str());

            st->visit(this);
        }
        if (!expr->isLastBlock())
        {
            std::stringstream str; str<< *expr->getTransition();

            if (_.getLogger()) _.getLogger()->logFormat(0, "\n;> %s\n", str.str().c_str());

            expr->getTransition()->visit(this);
        }
        else
            epilogue(*_functions[_currentFunction->id]);
    }

    void CodeGenerator::visit(IR::Variable const *const var) {
        RegOrMem rm;
        if (_varMeta[var->id].isReference) {
            // we should take the pointer itself! and write its value to _nextAssignment
            Reference ref = locateRef(_currentFunction->id, var->id);
            if (ref.type == Reference::REF_IMM) {
                RegOrMemMatchNoRedef(_nextAssignment,
                        _.lea(rax, ref.asMemCell());
                        _.mov(_nextAssignment.mem, rax);,
                        _.lea(_nextAssignment.gp, ref.asMemCell());,
                        throw CodeGenerationError("Should not write pointers into xmm registers!");
                )
                return;
            }
            else rm = ref.rm;
        }
        else {rm = locate(var);}

        RegOrMemMatchNoRedef (rm,
                {
                    RegOrMemMatchNoRedef(_nextAssignment,
                            _.mov(tempGpRegs[0], rm.mem);
                    _.mov(_nextAssignment.mem, tempGpRegs[0]);,
                    _.mov(_nextAssignment.gp, rm.mem);,
                    _.movq(_nextAssignment.xmm, rm.mem););
                },
                {
                    RegOrMemMatchNoRedef(_nextAssignment,
                            _.mov(_nextAssignment.mem,
                                    rm.gp),
                            _.mov(_nextAssignment.gp,
                                    rm.gp),
                            _.movq(_nextAssignment.xmm,
                                    rm.gp))
                },
                {
                    RegOrMemMatchNoRedef(_nextAssignment,
                            _.movq(_nextAssignment.mem,
                                    rm.xmm),
                            _.movq(_nextAssignment.gp,
                                    rm.xmm),
                            _.movq(_nextAssignment.xmm,
                                    rm.xmm))
                });
    }

    void CodeGenerator::visit(IR::Phi const *const expr) {
        throw BadIr("Use UnSsa to remove phi functions first");
    }

    void CodeGenerator::visit(IR::Int const *const expr) {
        RegOrMemMatch (_nextAssignment,
                _.mov(rax, expr->value); _.mov(mem, tempGpRegs[0]);,
                _.mov(gpReg, expr->value),
                _.mov(tempGpRegs[0], expr->value);
                _.movq(xmmReg, tempGpRegs[0]));
    }

    void CodeGenerator::visit(IR::Double const *const expr) {
        const int64_t value = D2I (expr->value);
        RegOrMemMatch (_nextAssignment,
                _.mov(mem, value),
                _.mov(gpReg, value),
                _.movq(xmmReg, ptr_abs(Ptr(_runtime.vivify(expr->value))));)
    }

    void CodeGenerator::visit(IR::Ptr const *const expr) {
        const int64_t value = contents(expr);
        RegOrMemMatch (_nextAssignment,
                _.mov(mem, value),
                _.mov(gpReg, value),
                _.mov(tempGpRegs[0], value);
                _.movq(xmmReg, tempGpRegs[0]));
    }

    void CodeGenerator::visit(IR::ReadRef const *const expr) {
        auto refLoc = locateRef(_currentFunction->id, expr->refId);
        if (refLoc.type == Reference::REF_RM) {
            RegOrMemMatchNoRedef(refLoc.rm,
                    RegOrMemMatch(_nextAssignment,
                            {_.mov(rax, refLoc.rm.mem); _.mov(rax, ptr(rax)); _.mov(mem, rax);},
                            {_.mov(rax, refLoc.rm.mem); _.mov(gpReg, ptr(rax));},
                            {_.mov(rax, refLoc.rm.mem); _.movq(xmmReg, ptr(rax));}),
                    RegOrMemMatch(_nextAssignment,
                    {_.mov(rax, ptr(refLoc.rm.gp)); _.mov(mem, rax);},
                    {_.mov(gpReg, ptr(refLoc.rm.gp));  },
                    {_.movq(xmmReg, ptr(refLoc.rm.gp));}),
                    throw CodeGenerationError("Reference can't reside in xmm register!"););
        }
        else if (refLoc.type == Reference::REF_IMM) {
            RegOrMemMatch(_nextAssignment, {_.mov(rax, refLoc.asMemCell()); _.mov(mem, rax);}, _.mov(gpReg, refLoc.asMemCell());, _.movq(xmmReg, refLoc.asMemCell()); )
        }
        else throw CodeGenerationError("Unknown reference type");
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
                if (expr->right->isVariable()
                        && virtRegOf(expr->right->asVariable()->id) ==
                        _nextAssignedVreg) {
                    genReducedBinOp(expr->type, _nextAssignment, expr->left, _, _runtime);
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
                if (expr->left->isVariable()
                        && virtRegOf(expr->left->asVariable()->id) == _nextAssignedVreg) {
                    {
                        genReducedBinOp(expr->type, _nextAssignment, expr->right, _, _runtime);
                        return;
                    }
                }
                genBinOp(expr->type, _nextAssignment, expr->left, expr->right, _, _runtime);
                return;
            default:
                throw
                        NotImplemented("native codegen for this operation is unsupported");
        };
    }

    void CodeGenerator::visit(const IR::UnOp *const unop) {
        genUnOp(unop->type, _nextAssignment, unop->operand, _, _runtime);
        return;
    }


    static bool shouldGetArgFromStack(std::vector<X86GpReg> const &savedRegs,
            std::vector<CodeGenerator::Function::Argument>
            const &arguments, size_t argIdx,
            const IR::Atom *source,
            CodeGenerator &generator) {
        if (!source->isVariable())
            return false;
        auto rm = generator.locate(source->asVariable()->id);
        if (!rm.isGp())
            return false;
        for (size_t i = 0; i < argIdx; i++)
            if (arguments[i].class_ ==
                    CodeGenerator::AC_INTEGER && arguments[i].gpReg == rm.gp)
                return true;
        return false;
    }

    static uint32_t locateGpRegArgSource(std::vector<X86GpReg> const &savedRegs,
            std::vector< CodeGenerator::Function::Argument> const &arguments,
            size_t argIdx,
            const IR::Atom *source,
            CodeGenerator &generator) {

        auto rm = generator.locate(source->asVariable()->id);
        for (uint32_t i = 0; i < argIdx; i++)
            if (arguments[i].class_ ==
                    CodeGenerator::AC_INTEGER && arguments[i].gpReg == rm.gp) {
                //i is an offset/8
                long savedRegsIdx =
                        std::find(savedRegs.cbegin(), savedRegs.cend(),
                                rm.gp) - savedRegs.cbegin();
                return savedRegs.size() - 1 - (uint32_t) savedRegsIdx;    // index of this reg from end
            }
        throw TranslationError("locateGpRegArgSource should be called only when shouldGetArgFromStack condition holds");
    }

    void CodeGenerator::visit(const IR::Call *const expr) {
        IR::Function const& called = _functions[expr->funId]->function;
        if (_.getLogger()) _.getLogger()->logFormat(0, "; Calling function id %d name %s \n", called.id, called.name.c_str() );
        auto args = expr->params;
        //fixme HACK get RID OF MEMLEAK PELASE
        for( auto refp : expr->refParams) args.push_back(new IR::Variable(refp));
        genCall(*_functions[expr->funId], args );
    }


    void CodeGenerator::visit(const IR::Print *const expr) {
        std::vector<IR::Atom const *> params;
        params.push_back(expr->atom);
        switch (_deriver.visitExpression(expr->atom)) {
            case IR::VT_Int:
                genCall(*_f_printInt, params);
                break;
            case IR::VT_Double:
                genCall(*_f_printDouble, params);
                break;
            case IR::VT_Ptr:
                genCall(*_f_printStr, params);
                break;
            default:
                throw BadIr("Printing an atom of invalid type");
        }
    }


    void CodeGenerator::visit(const IR::JumpAlways *const expr) {
        if (expr->destination != nextBlock()) _.jmp(_blocks[expr->destination].label);
    }

    void CodeGenerator::visit(const IR::JumpCond *const expr) {
        FOR_ATOM (expr->condition,
                genJmp((contents(aInt)) ? expr->yes : expr->no);,
                genJmp((contents(aDouble)) ? expr->yes : expr->no);,
        genJmp((contents(aPtr)) ? expr->yes : expr->no);,
                {
                    RegOrMemMatch(locate(aVar),
                            _.mov(rax, mem);
                    _.test(rax, rax);
                    , _.test(gpReg, gpReg);
                    ,
                    _.movq(rax, xmmReg);
                    _.test(rax, rax););
                    _.jz(_blocks[expr->no].label);
                    genJmp(expr->yes);
                },
                {
                    (void) aReadRef;
                    throw NotImplemented("readref");
                };);
    }

    void CodeGenerator::visit(const IR::WriteRef *const expr) {
        bool assigned = false;
        Reference refLoc = locateRef(_currentFunction->id, expr->refId);
        if (refLoc.type == Reference::REF_IMM) {

            _nextAssignment =  RegOrMem( rax );
            expr->atom->visit(this);
            _.mov(refLoc.asMemCell(), rax);
            return;
        }
        //REF_RM
        const bool sse = typeOf(expr->refId) == IR::VT_Double;
        const bool saveRbx = isRegAliveAfter(expr, rbx);
        RegOrMemMatch(refLoc.rm, {
            if (sse) _nextAssignment = RegOrMem(xmm0); else _nextAssignment = RegOrMem(rax);
            expr->atom->visit(this);
            if (saveRbx) _.push(rbx);
            _.mov(rbx, mem);
            if (sse) _.movq(ptr(rbx), xmm0); else _.mov(ptr(rbx), rax);
            if (saveRbx) _.pop(rbx);
        },
                { _nextAssignment = RegOrMem(X86Mem(ptr(gpReg)));
//                    if (sse) _nextAssignment = RegOrMem(xmm0); else _nextAssignment = RegOrMem(rax);
                    expr->atom->visit(this);
//                    if (sse) _.movq(ptr(gpReg), xmm0); else _.mov(ptr(gpReg), rax);

                }, {
            (void)xmmReg;
            throw CodeGenerationError("Xmm register can't hold reference! ref id : " + to_string(expr->refId));
        });


//
//        if (rmi.type == Reference::REF_IMM) _nextAssignment = RegOrMem(X86Mem(ptr(rbp, rmi.imm)));
//        else if (rmi.type == Reference::REF_RM) {
//            RegOrMemMatch(rmi.rm, {
//                _.push(rax);
//                const bool sse = typeOf(expr->refId) == IR::VT_Double;
//                _nextAssignment = sse? RegOrMem(xmm0) : RegOrMem(rax);
//                expr->atom->visit(this);
//                if (isRegAliveAfter(expr, rbx)) _.push(rbx);
//                _.mov(rbx, rmi.rm.mem);
//                if (sse) _.movq(ptr(rbx), xmm0);  else _.mov(ptr(rbx), rax);
//                if (isRegAliveAfter(expr, rbx)) _.pop(rbx);
//                assigned = true;
//            }, {
//                _nextAssignment = RegOrMem(X86Mem(ptr(gpReg)));
//            }, {
//                throw CodeGenerationError("Xmm register can't hold reference! ref id : " + to_string(expr->refId));
//            })
//        }


       if (!assigned) expr->atom->visit(this);
    }


    bool CodeGenErrorHandler::handleError(asmjit::Error code,
            const char *message) {
        _debug << message << std::endl;
        return true;
    }

    void *CodeGenerator::translate() {
        _debug <<
                "\n-------------------------------\n   Code generation has started \n-------------------------------\n";


        StringLogger logger;
        _.setErrorHandler(new CodeGenErrorHandler(_debug));
        _.setLogger(&logger);

        for (auto f:_functions)
            f->setup();
        for (auto f:_ir.functions)
            visit(f);

        void *result = _.make();
        _debug << "   Generated code:\n" << logger.getString() << std::endl;

        if (result)
            return result;
        return NULL;
    }

    X86XmmReg const &CodeGenerator::xmmFromVirt(uint64_t vreg) const {
        if (vreg == gpAccVreg)
            throw CodeGenerationError ("attempt to use gp acc register to store a double");
        if (vreg == doubleAccVreg)
            return xmm0;
        return allocableXmmRegs[_functions.at(_currentFunction->id)->
                regsXmm[vreg]];
    }

    X86GpReg const &CodeGenerator::gpFromVirt(uint64_t funId, uint64_t vreg) const {
        if (vreg == gpAccVreg)
            return rax;
        if (vreg == doubleAccVreg)
            throw CodeGenerationError ("attempt to use xmm acc register to store a non-double");
        return allocableRegs[_functions.at(funId)->regsGp[vreg]];
    }

    X86GpReg const &CodeGenerator::gpFromVirt(uint64_t vreg) const {
        return gpFromVirt(_currentFunction->id, vreg);
    }

    RegOrMem CodeGenerator::locate(IR::VarId id) const {
        if (id == gpAccVar) return RegOrMem(rax);
        if (id == doubleAccVar) return RegOrMem(xmm0);

        CodeGenerator::Function* f = _functions.at(_currentFunction->id);
        //parameters in xmm registers are copied into stack
        Function::Argument const *const arg =f->argumentForVar(id);
        if (arg)
            switch (arg->class_) {
                case AC_INTEGER:
                    return RegOrMem(arg->gpReg);
                case AC_SSE:
                    if (arg->xmmReg== xmm0) return RegOrMem(X86Mem(rbp, f->backedXmmArgsRegsRbpOffsets[0]));
                    if (arg->xmmReg== xmm1) return RegOrMem(X86Mem(rbp, f->backedXmmArgsRegsRbpOffsets[1]));
                    return RegOrMem(arg->xmmReg);
                case AC_MEM:
                    return RegOrMem(X86Mem(rbp, arg->getRbpOffset()));
                default:
                    std::stringstream msg;
                    msg << "Can't locate var " << id;
                    throw CodeGenerationError(msg.str());
            }
        else {
            auto vreg = virtRegOf(id);
            if (vreg == gpAccVreg)
                return RegOrMem(rax);
            if (vreg == doubleAccVreg)
                return RegOrMem(xmm0);
            FOR_INT_DOUBLE (typeOf(id), return RegOrMem(gpFromVirt(vreg));, return RegOrMem(xmmFromVirt(vreg));
            )
        }

    }

    bool CodeGenerator::isRegAliveAfter(const IR::Statement *const statement,
            X86GpReg const &reg) {
        size_t *regs = _functions.at(_currentFunction->id)->regsGp;
        for (size_t vreg = 0; vreg < ALLOCABLE_REGS_COUNT; vreg++)
            if (allocableRegs[regs[vreg]].getId() == reg.getId()) {
                auto regIt = allocations().vregToVarId.find(vreg);
                if (regIt == allocations().vregToVarId.end())
                    return false;
                IR::VarId varid = (*regIt).second;
                return liveInfo->data.at(_currentFunction->id)->varIntervals.
                        at(varid).contains(statement->num + 1);
            }
        throw CodeGenerationError("This register is not allocable");
    }

    bool CodeGenerator::isRegAliveAfter(const IR::Statement *const statement,
            X86XmmReg const &reg) {
        size_t *regs = _functions.at(_currentFunction->id)->regsXmm;
        for (size_t vreg = 0; vreg < ALLOCABLE_REGS_COUNT; vreg++)
            if (allocableRegs[regs[vreg]].getId() == reg.getId()) {
                auto regIt = allocations().vregToVarId.find(vreg);
                if (regIt == allocations().vregToVarId.end())
                    return false;
                IR::VarId varid = (*regIt).second;
                return liveInfo->data.at(_currentFunction->id)->varIntervals.
                        at(varid).contains(statement->num + 1);
            }
        throw CodeGenerationError("This register is not allocable");
    }

    CodeGenerator::Info::Info() {
        for (size_t i = 0; i < INTEGER_ARGUMENTS_REGS_LENGTH; i++)
            nextIntArg.
                    push_back(std::
            find(allocableRegs, std::end(allocableRegs),
                    integerArgumentsRegs[i]) - allocableRegs);
        for (size_t i = 0; i < SSE_ARGUMENTS_REGS_LENGTH; i++)
            nextSseArg.
                    push_back(std::
            find(allocableXmmRegs, std::end(allocableXmmRegs),
                    sseArgumentsRegs[i]) - allocableXmmRegs);
    }


    void CodeGenerator::genAssign(RegOrMem const &from, RegOrMem const &to) {
        if (from == to)
            return;
        RegOrMemMatchNoRedef (from,
                RegOrMemMatchNoRedef(to,
                        _.mov(rax, from.mem);
                _.mov(to.mem, rax);
                ,
                _.mov (to.gp, from.mem),
                _.movq (to.xmm, from.mem)),
                RegOrMemMatchNoRedef(to, _.mov(to.mem, from.gp);
                , _.mov (to.gp, from.gp),
                _.movq (to.xmm, from.gp)),
                RegOrMemMatchNoRedef(to, _.movq(to.mem, from.xmm);
                , _.movq (to.gp, from.xmm),
                _.movq (to.xmm, from.xmm);
        ))
    }

    bool CodeGenerator::isCallerSave(X86GpReg const &reg) {
        return
                std::find(callerSave, std::end(callerSave), reg) !=
                        std::end(callerSave);
    }


    bool CodeGenerator::isCalleeSave(X86GpReg const &reg) {
        return
                std::find(calleeSave, std::end(calleeSave), reg) !=
                        std::end(calleeSave);
    }

    void CodeGenerator::Function::setupRegisterMapping() {
        bool assignedGp[ALLOCABLE_REGS_COUNT] = {false};
        bool assignedXmm[ALLOCABLE_REGS_COUNT] = {false};

        std::set<size_t> freeGpRegs;
        std::set<size_t> freeXmmRegs;

        for (size_t i = 0; i < ALLOCABLE_REGS_COUNT; i++)
            freeGpRegs.insert(i);
        for (size_t i = 0; i < ALLOCABLE_REGS_COUNT; i++)
            freeXmmRegs.insert(i);

        for (Argument const &arg :arguments) {
            auto vreg = generator.virtRegOf(function.id, arg.var);

            assert (vreg != generator.gpAccVreg);
            assert (vreg != generator.doubleAccVreg);

            switch (arg.class_) {
                case AC_INTEGER: {
                    auto regIdx =
                            std::find(allocableRegs, std::end(allocableRegs),
                                    arg.gpReg) - allocableRegs;
                    regsGp[vreg] = regIdx;
                    assignedGp[vreg] = true;
                    freeGpRegs.erase(regIdx);
                    break;
                }
                case AC_SSE: {
                    auto regIdx =
                            std::find(allocableXmmRegs, std::end(allocableXmmRegs),
                                    arg.xmmReg) - allocableXmmRegs;
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
        generator._debug << "GP reg mappings for functions" << function.
                id << ":\n";
        for (size_t i = 0; i < ALLOCABLE_REGS_COUNT; i++) {
            generator._debug << i << "->" << regsGp[i] << "\n";
        }
        generator._debug << "Xmm reg mappings for function " << function.
                id << ":\n";
        for (size_t i = 0; i < ALLOCABLE_REGS_COUNT; i++) {
            generator._debug << i << "->" << regsXmm[i] << "\n";
        }
    }

    void CodeGenerator::Function::setupArguments() {
        size_t intArgs = 0, sseArgs = 0;
        uint32_t memArgs = 0;

        for (size_t i = 0; i < function.arguments(); ++i) {
            auto argId = function.argument(i);
            const IR::VarType type = generator.typeOf(argId);
            auto const cls = CodeGenerator::argumentClassForType(type);
            if (generator._varMeta[argId].isReference || cls == AC_INTEGER) {
                if (intArgs < INTEGER_ARGUMENTS_REGS_LENGTH)
                    arguments.push_back(Argument(i, argId, type, integerArgumentsRegs[intArgs], generator._varMeta[argId].isReference));
                else
                    arguments.push_back(Argument(i, argId, type, memArgs++, generator._varMeta[argId].isReference));
                intArgs++;
            }
            else if (cls == AC_SSE) {
                if (sseArgs < SSE_ARGUMENTS_REGS_LENGTH)
                {
                    arguments.push_back(Argument(i, argId, type, sseArgumentsRegs[sseArgs], generator._varMeta[argId].isReference ));
                    //back up xmm0 and xmm1
                    if (sseArgs < 2)
                        backedXmmArgsRegsRbpOffsets.push_back((int32_t) (function.memoryCells.size()*8 + sseArgs*8));
                }
                else
                    arguments.push_back(Argument(i, argId, type, memArgs++, generator._varMeta[argId].isReference ));
                sseArgs++;
            }
            else
                arguments.push_back(Argument(i, argId, type, memArgs++, generator._varMeta[argId].isReference ));
        }
    }

    std::vector<X86GpReg> CodeGenerator::liveRegs(size_t statementIdx) const {
        std::vector<X86GpReg> regs;
        for (auto &interval:liveInfo->data[_currentFunction->id]->varIntervals)
            if (interval.second.contains(statementIdx)) if (typeOf(interval.first) != IR::VT_Double)
                regs.push_back(gpFromVirt(virtRegOf(interval.first)));
        return regs;
    }

    std::set<X86GpReg, RegComp> CodeGenerator::usedGpRegs(IR::Function const
    &f) const {
        std::set<X86GpReg, RegComp> regs;
        for (auto varToInt:liveInfo->data[f.id]->varIntervals)
            if (typeOf(varToInt.first) == IR::VT_Int ||
                    typeOf(varToInt.first) == IR::VT_Ptr)
                regs.insert(gpFromVirt(f.id, virtRegOf(f.id, varToInt.first)));
        return regs;
    }

    std::vector<X86GpReg> CodeGenerator::usedGpCalleeSave(IR::Function const
    &f) const {
        std::vector<X86GpReg> regs;
        for (auto reg:usedGpRegs(f))
            if (isCalleeSave(reg))
                regs.push_back(reg);
        return regs;
    }

    std::vector<X86GpReg> CodeGenerator::liveCallerSaveRegs(size_t statementIdx) const {
        std::vector<X86GpReg> result;
        auto lives = liveRegs(statementIdx);
        for (auto &l:lives)
            if (isCallerSave(l))
                result.push_back(l);
        return result;
    }

    std::vector<X86GpReg> CodeGenerator::usedXmmRegs(IR::Function const &f) const {
        std::vector<X86GpReg> regs;
        for (auto varToInt:liveInfo->data[f.id]->varIntervals)
            if (typeOf(varToInt.first) == IR::VT_Double)
                regs.push_back(gpFromVirt(virtRegOf(f.id, varToInt.first)));
        return regs;
    }

    std::vector<X86XmmReg> CodeGenerator::liveXmmRegs(size_t statementIdx) const {
        std::vector<X86XmmReg> regs;
        for (auto &interval:liveInfo->data[_currentFunction->id]->varIntervals)
            if (interval.second.contains(statementIdx)) if (typeOf(interval.first) == IR::VT_Double)
                regs.push_back(xmmFromVirt(virtRegOf(interval.first)));
        return regs;
    }

    static void restoreXmm(std::vector<X86XmmReg> const &savedXmmRegs, X86Assembler &_) {
        if (savedXmmRegs.size() == 0) return;

        int32_t offset = (savedXmmRegs.size() - 1) * 8;
        for (auto &reg : savedXmmRegs) {
            _.movq(reg, ptr(rsp, offset));
            offset -= 8;
        }
        _.add(rsp, savedXmmRegs.size() * 8);
    }


    void CodeGenerator::genCall(CodeGenerator::Function const &f, std::vector<IR::Atom const *> const &args) {

        RegOrMem writeResultInto = _nextAssignment;

        //first save all registers we need
        std::vector<X86GpReg> savedRegs;
        for(auto& r :  liveCallerSaveRegs(_currentStatement->num))
            if (f.returnType == IR::VT_Unit || ( _nextAssignment.isGp() && _nextAssignment.gp != r ))
                savedRegs.push_back(r);

        std::vector<X86XmmReg> savedXmmRegs;
        for(auto& xr : liveXmmRegs(_currentStatement->num))
            if (f.returnType == IR::VT_Unit || ( _nextAssignment.isGp() && _nextAssignment.xmm != xr ))
                savedXmmRegs.push_back(xr);
        //after the call we are not 16-aligned;
        //then used gp (callee save)
        //if stack frame is present, rbp is pushed there.
        //then caller save regs are saved and after an offset (0 or 8, which we are trying to determine here)
        //memory arguments are pushed.

        const bool extraPush =
                (!_functions[_currentFunction->id]->isStackAlignedAfterPrologue()
                        + _functions[_currentFunction->id]->backedXmmArgsRegsRbpOffsets.size()
                        + savedRegs.size()
                        + savedXmmRegs.size() + f.memArgsCount()) % 2 == 1;

        for (auto const &reg: savedRegs)
            _.push(reg);

        //fixme A better save without pushes.
        for (auto const &xmmReg:savedXmmRegs) {
            _.movq(rax, xmmReg);
            _.push(rax);
        }

        if (extraPush)
            _.sub(rsp, 8);

        //then insert arguments
        for (size_t i = 0; i < f.arguments.size(); i++) {
            switch (f.arguments[i].class_) {
                case AC_INTEGER:
                    if (shouldGetArgFromStack(savedRegs, f.arguments, i, args[i], *this))
                        _.mov(f.arguments[i].gpReg, ptr(rsp,8 * (locateGpRegArgSource(savedRegs, f.arguments, i, args[i],*this) + extraPush)));
                    else {
                        _nextAssignment = RegOrMem(f.arguments[i].gpReg);
                        args[i]->visit(this);
                    }
                    break;
                case AC_SSE:
                    _nextAssignment = RegOrMem(f.arguments[i].xmmReg);
                    args[i]->visit(this);
                    break;
                case AC_MEM:
                    _nextAssignment = RegOrMem(rax);
                    _.push(rax);
                    args[i]->visit(this);
                    break;
            }
        }
        //todo reference parameters!

        //emit call
        if (!f.function.isNative())
            _.call(f.label);
        else
            _.call(Ptr(f.function.nativeAddress));

        //restore mem args and alignment from stack
        const size_t memArgsAndAlign = extraPush + f.memArgsCount();
        if (memArgsAndAlign)
            _.add(rsp, memArgsAndAlign * 8);


        //restore registers
        for (auto it = savedRegs.rbegin(); it != savedRegs.rend(); it++)
            _.pop(*it);


//        for (auto it = savedXmmRegs.rbegin(); it != savedXmmRegs.rend(); it++) {
//            _.pop(rax);
//            _.movq(*it, rax);
//        }

        restoreXmm(savedXmmRegs, _);
        const auto retType = f.returnType;
        if (retType == IR::VT_Unit) {
        }
        else if (retType == IR::VT_Double)
            genAssign(RegOrMem(xmm0), writeResultInto);
        else
            genAssign(RegOrMem(rax), writeResultInto);



    }



    CodeGenerator::Function const *CodeGenerator::makePredefFunction(void *address, IR::VarType arg, const std::string& name ) {
        IR::Function *f = new IR::Function(UINT16_MAX, address, IR::VT_Unit, name );
        f->parametersIds.push_back(makeVar(arg));
        auto fun = new CodeGenerator::Function(*this, *f);
        if (arg == IR::VT_Double)fun->arguments.push_back(Function::Argument(0, f->parametersIds[0], arg, sseArgumentsRegs[0], false));
        else fun->arguments.push_back(Function::Argument(0, f->parametersIds[0], arg, integerArgumentsRegs[0], false));
        _functions.push_back(fun);
        return fun;
    }

    void CodeGenerator::setupPredefFunctions() {

        _f_printInt = makePredefFunction((void *) print_int, IR::VT_Int, "print_int");
        _f_printDouble = makePredefFunction((void *) print_double, IR::VT_Double, "print_double");
        _f_printStr = makePredefFunction((void *) print_str, IR::VT_Ptr, "print_string");

    }

    IR::Block const *CodeGenerator::nextBlock() const {
        auto& blocks = liveInfo->data[_currentFunction->id]->orderedBlocks;
        auto it = std::find(blocks.cbegin(), blocks.cend(), _currentBlock);
        if (it == blocks.cend()) throw TranslationError("Current block is not inside ordered blocks of liveinfo!");
        it++;
        if (it == blocks.cend()) return NULL;
        return *it;
    }

    Reference CodeGenerator::locateRef(uint64_t funId, IR::VarId refId) const {
        auto f  = _functions[funId];
        auto refArg = f->refIndexOf(refId);
        if (refArg) return Reference(refArg->location());
        int32_t idx = 0;
        for (auto mcId : f->function.memoryCells) {
            if (mcId == refId) return Reference(-8 - 8*idx);
            idx++;
        }
        throw BadIr("Can't find reference id " + to_string(refId));
    }
}
