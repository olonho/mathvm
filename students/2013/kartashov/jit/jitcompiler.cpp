#include "jitcompiler.h"
#include "parser.h"

#include <dlfcn.h>
#include <sstream>

using namespace mathvm;
using namespace AsmJit;

#define MEM_PTR     0
#define STACK_PTR   1

Status *JITCompiler::translate(const string &program, Code **code) {
    Parser p;
    Status *s = p.parseProgram(program);
    if(s && s->isError()) return s;

    try {
        analyzer.analyze(p.top());
    } catch(std::logic_error e) {
        return new Status(e.what());
    }

    this->code = new AsmJitCodeImpl();
    this->code->getFuncPtrs().resize(analyzer.getAllMetadata().size());
    *code = this->code;
    currentScope = 0;
    currentNode = 0;

    try {
        compileFunction(p.top());
    } catch (std::logic_error e) {
        return new Status(e.what(), currentNode ? currentNode->position() : 0);
    }

    return 0;
}

//---------------------------------------------------------------------------------------------

void JITCompiler::compileFunction(AstFunction *f) {
    AsmJitFunction *mcf = code->functionByName(f->name());
    if(!mcf) {
        mcf = new AsmJitFunction(f, debugInfo);
        code->addFunction(mcf);
        mcf->setVarsToStore(analyzer[mcf->id()]->varsToStore);
    }
    enterScope(mcf);

    FunctionDescription *fd = analyzer[mcf->id()];

    //store vars in signature order
    for(Signature::const_iterator i = mcf->signature().begin() + 1; i != mcf->signature().end(); ++i) {
        AstVar *v = f->scope()->lookupVariable(i->second);
        currentScope->addVar(v);
    }

    //return_type function(memory_ptr, stack_ptr)
    switch(f->returnType()) {
    case VT_INT : case VT_STRING:
        currentCode().newFunction(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, void*, void*>());
        mcf->retVar() = createGPVar(f->returnType());
        break;
    case VT_DOUBLE:
        currentCode().newFunction(CALL_CONV_DEFAULT, FunctionBuilder2<double, void*, void*>());
        mcf->retVar() = createXMMVar(VT_DOUBLE);
        break;
    case VT_VOID:
        currentCode().newFunction(CALL_CONV_DEFAULT, FunctionBuilder2<void, void*, void*>());
        break;
    default:
        break;
    }
    currentCode().getFunction()->setHint(FUNCTION_HINT_NAKED, true);

    if(fd->hasIntPrintNode) mcf->intPrintBuffer() = createGPVar(VT_INT);
    if(fd->canReuseMathVars) {
        mcf->intLiteralBuffer() = createGPVar(VT_INT);
        mcf->intVarBuffer() = createGPVar(VT_INT);
    }
    f->node()->visit(this);
    if(fd->hasIntPrintNode) currentCode().unuse(*mcf->intPrintBuffer().gp);
    if(fd->canReuseMathVars) {
        currentCode().unuse(*mcf->intLiteralBuffer().gp);
        currentCode().unuse(*mcf->intVarBuffer().gp);
    }

    //function translated
    currentCode().bind(mcf->retLabel());
    switch(f->returnType()) {
    case VT_INT: case VT_STRING:
        currentCode().ret(*mcf->retVar().gp);
        break;
    case VT_DOUBLE:
        currentCode().ret(*mcf->retVar().xmm);
        break;
    default:
        currentCode().ret();
    }
    currentCode().endFunction();

    void *compiled = (void*)currentCode().make();
    if(!compiled) {
        std::stringstream ss; ss << "AsmJit compilation error: " << currentCode().getError();
        throw std::logic_error(ss.str());
    }
    code->setCompiled(mcf->id(), compiled);

    if(debugInfo) std::cout << "Compiled function: " << f->name() << std::endl;

    exitScope();
}

//---------------------------------------------------------------------------------------------

void JITCompiler::convertTOSType(VarType from, VarType to) {
    if(from == to) return;

    if(from == VT_INT && to == VT_DOUBLE) {
        AsmJitVar intVar = popCurrentStack();
        AsmJitVar doubleVar = createXMMVar(VT_DOUBLE);
        currentCode().cvtsi2sd(*doubleVar.xmm, *intVar.gp);
        pushCurrentStack(doubleVar);
        currentCode().unuse(*intVar.gp);
    } else if(from == VT_DOUBLE && to == VT_INT) {
        AsmJitVar intVar = createGPVar(VT_INT);
        AsmJitVar doubleVar = popCurrentStack();
        if(doubleVar.type != VT_DOUBLE) throw std::logic_error("stack corrupted");
        currentCode().cvttsd2si(*intVar.gp, *doubleVar.xmm);
        pushCurrentStack(intVar);
        currentCode().unuse(*doubleVar.xmm);
    } else if(from == VT_STRING && to == VT_INT) {
        return;
    } else {
        throw std::logic_error(std::string("Unsupported type conversion: ") + typeToName(from) + " to " + typeToName(to));
    }
}

//---------------------------------------------------------------------------------------------

void JITCompiler::storeVar(const AstVar *var) {
    Compiler &c = currentCode();
    uint32_t varIndex = analyzer[currentScope->function->id()]->getAddress(var->name());

    if(typeStack.empty()) throw std::logic_error("Empty stack detected");
    if(var->type() != typeStack.top()) convertTOSType(typeStack.top(), var->type());
    //No need to support BC_STORE_X_VAR_N
    switch(var->type()) {
    case VT_STRING: case VT_INT: {
        AsmJitVar val = popCurrentStack();
        c.mov(ptr(c.argGP(0), sizeof(int64_t) * varIndex), *val.gp);
        if(!currentScope->varReuseMode) c.unuse(*val.gp);
        else currentScope->varReuseMode = false;
        break;
    }
    case VT_DOUBLE: {
        AsmJitVar val = popCurrentStack();
        c.movq(ptr(c.argGP(0), sizeof(int64_t) * varIndex), *val.xmm);
        c.unuse(*val.xmm);
        break;
    }
    default:
        throw std::logic_error("Unsupported type to store");
    }
}

void JITCompiler::loadVar(const AstVar *var) {
    Compiler &c = currentCode();
    AsmJitFunction *fn = currentScope->function;
    uint32_t varIndex = analyzer[fn->id()]->getAddress(var->name());

    switch(var->type()) {
    case VT_STRING: case VT_INT: {
        Mem valAddr = ptr(c.argGP(0), sizeof(int64_t) * varIndex);
        if(currentScope->inPrintNode) {
            AsmJitVar &printBuffer = fn->intPrintBuffer();
            c.mov(*printBuffer.gp, valAddr);
            printBuffer.type = var->type();
            pushCurrentStack(printBuffer);
        } else if(currentScope->varReuseMode) {
            c.mov(*fn->intVarBuffer().gp, valAddr);
            pushCurrentStack(fn->intVarBuffer());
        } else {
            pushCurrentStack(createGPVar(var->type(), valAddr));
        }
        break;
    }
    case VT_DOUBLE: {
        Mem valAddr = ptr(c.argGP(0), sizeof(int64_t) * varIndex);
        pushCurrentStack(createXMMVar(valAddr));
        break;
    }
    default:
        throw std::logic_error("Unsupported type to load");
    }
}

void JITCompiler::loadIntConst(int64_t val, bool replace) {
    if(replace) currentCode().mov(*currentStack().top().gp, imm(val));
    else pushCurrentStack(createGPVar(VT_INT, val));
}

void JITCompiler::loadDoubleConst(double val) {
    pushCurrentStack(createXMMVar(val));
}

//---------------------------------------------------------------------------------------------

void JITCompiler::visitFunctionNode(FunctionNode *node) {
    if(node->body()->nodeAt(0)->isNativeCallNode())  node->body()->nodeAt(0)->visit(this);
    else node->body()->visit(this);
}

void JITCompiler::visitBlockNode(BlockNode *node) {
    currentNode = node;

    Scope::VarIterator vars(node->scope());
    while(vars.hasNext()) currentScope->addVar(vars.next());

    //declare scope functions first
    Scope::FunctionIterator funcs(node->scope());
    while(funcs.hasNext()) {
        AsmJitFunction *mcf = new AsmJitFunction(funcs.next(), debugInfo);
        code->addFunction(mcf);
        mcf->setVarsToStore(analyzer[mcf->id()]->varsToStore);
    }

    //now we can compile scope functions
    funcs = Scope::FunctionIterator(node->scope());
    while(funcs.hasNext()) compileFunction(funcs.next());

    for(uint32_t i = 0; i < node->nodes(); ++i) node->nodeAt(i)->visit(this);
}

//---------------------------------------------------------------------------------------------

void JITCompiler::pushVarOnFunctionStack(AsmJitVar var) {
    Compiler &c = currentCode();

    switch(var.type) {
    case VT_STRING: case VT_INT:
        c.mov(ptr(c.argGP(STACK_PTR)), *var.gp);
        c.unuse(*var.gp);
        break;
    case VT_DOUBLE:
        c.movq(ptr(c.argGP(STACK_PTR)), *var.xmm);
        c.unuse(*var.xmm);
        break;
    default: throw std::logic_error("bad argument");
    }
    c.add(c.argGP(STACK_PTR), imm(sizeof(int64_t)));
}

void JITCompiler::visitCallNode(CallNode *node) {
    currentNode = node;
    Compiler &c = currentCode();
    AsmJitFunction *f = code->functionByName(node->name());
    if(!f) throw std::logic_error(std::string("function '") + node->name() + "' is not defined");
    FunctionDescription &fmd = analyzer.getMetadata(f->id());

    //create callee stack in the next memory block
    //push parametes
    for(size_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        convertTOSType(tosType(), f->parameterType(i));
        pushVarOnFunctionStack(popCurrentStack());
    }

    //push closures
    for(VarUsageList::const_iterator it = fmd.closureVars.begin(); it != fmd.closureVars.end(); ++it) {
        const AstVar *v = currentScope->findVar(it->var, it->funcId);
        loadVar(v);
        pushVarOnFunctionStack(popCurrentStack());
    }

    //initialize locals
    for(size_t i = 0; i != fmd.locals.size(); ++i) {
        switch(fmd.localTypes[i]) {
        case VT_INT: case VT_STRING:
            loadIntConst(0);
            pushVarOnFunctionStack(popCurrentStack());
            break;
        case VT_DOUBLE:
            loadDoubleConst(0.0);
            pushVarOnFunctionStack(popCurrentStack());
            break;
        default:
            throw std::logic_error("bad argument type");
        }
    }

    //now we can call the function
    AsmJitVar funcPtr = createGPVar(VT_INT, (size_t)(&code->getFuncPtrs()[f->id()]));
    AsmJitVar memPtr = createGPVar(VT_INT);
    AsmJitVar stackPtr = createGPVar(VT_INT);
    //set memory base and next memory block pointer for callee
    c.mov(*memPtr.gp, c.argGP(STACK_PTR));
    c.sub(*memPtr.gp, imm(sizeof(int64_t) * fmd.varsToStore));
    c.mov(*stackPtr.gp, c.argGP(STACK_PTR));
    ECall *funcCall = c.call(ptr(*funcPtr.gp));

    switch(f->returnType()) {
    case VT_INT: case VT_STRING: {
        AsmJitVar funcRes = createGPVar(f->returnType());
        funcCall->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, void*, void*>());
        funcCall->setReturn(*funcRes.gp);
        pushCurrentStack(funcRes);
        break;
    }
    case VT_DOUBLE: {
        AsmJitVar funcRes = createXMMVar(VT_DOUBLE);
        funcCall->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<double, void*, void*>());
        funcCall->setReturn(*funcRes.xmm);
        pushCurrentStack(funcRes);
        break;
    }
    default:
        funcCall->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<void, void*, void*>());
        break;
    }
    funcCall->setArgument(MEM_PTR, *memPtr.gp);
    funcCall->setArgument(STACK_PTR, *stackPtr.gp);
    c.unuse(*funcPtr.gp);
    c.unuse(*memPtr.gp);
    c.unuse(*stackPtr.gp);

    //next we need to pop all the calling stuff from current function 'stack'
    //for locals and params just sub var count from current STACK_PTR val

    //pop locals
    if(!fmd.locals.empty()) c.sub(c.argGP(STACK_PTR), imm(sizeof(int64_t) * fmd.locals.size()));

    //pop and save externals
    for(VarUsageList::const_reverse_iterator rit = fmd.closureVars.rbegin(); rit != fmd.closureVars.rend(); ++rit) {
        uint16_t addrInCurrentFunc = analyzer[currentScope->function->id()]->getAddress(rit->var, rit->funcId);
        GPVar val(c.newGP());
        c.mov(val, ptr(c.argGP(STACK_PTR), -sizeof(int64_t)));
        c.mov(ptr(c.argGP(MEM_PTR), sizeof(int64_t) * addrInCurrentFunc), val);
        c.unuse(val);
        c.sub(c.argGP(STACK_PTR), imm(sizeof(int64_t)));
    }

    //pop params
    if(!fmd.params.empty()) c.sub(c.argGP(STACK_PTR), imm(sizeof(int64_t) * f->parametersNumber()));
}

void JITCompiler::visitNativeCallNode(NativeCallNode *node) {
    currentNode = node;
    Compiler &c = currentCode();

    void *addr = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
    if(!addr) throw std::logic_error("Native function '" + node->nativeName() + "' not found");

    std::vector<AsmJitVar> nativeParams;
    Signature::const_reverse_iterator signEnd = --node->nativeSignature().rend();
    for(Signature::const_reverse_iterator it = node->nativeSignature().rbegin(); it != signEnd; ++it) {
        switch(it->first) {
        case VT_INT: case VT_STRING:
            nativeParams.push_back(createGPVar(it->first));
            break;
        case VT_DOUBLE:
            nativeParams.push_back(createXMMVar(VT_DOUBLE));
            break;
        default:
            throw std::logic_error(std::string("Unsupported native funcction parameter type: ") + typeToName(it->first));
        }
    }

    FunctionBuilderX nativeBuilder;
    for(size_t i = 0; i < nativeParams.size(); ++i) {
        switch(nativeParams[i].type) {
        case VT_DOUBLE:
            nativeBuilder.addArgument<double>();
            c.movq(*nativeParams[i].xmm, ptr(c.argGP(STACK_PTR), -sizeof(uint64_t) * (nativeParams.size() - i)));
            break;
        default:    //all unsupported types were throwed in the code above
            nativeBuilder.addArgument<int64_t>();
            c.mov(*nativeParams[i].gp, ptr(c.argGP(STACK_PTR), -sizeof(uint64_t) * (nativeParams.size() - i)));
            break;
        }
    }

    VarType retType = node->nativeSignature().front().first;
    switch(retType) {
    case VT_INT: case VT_STRING: nativeBuilder.setReturnValue<int64_t>(); break;
    case VT_DOUBLE: nativeBuilder.setReturnValue<double>(); break;
    case VT_VOID: nativeBuilder.setReturnValue<void>(); break;
    default:
        throw std::logic_error("Unsupported return type for native function");
    }

    ECall *nativeCall = c.call(imm((size_t)addr));
    nativeCall->setPrototype(CALL_CONV_DEFAULT, nativeBuilder);

    for(size_t i = 0; i < nativeParams.size(); ++i) {
        switch(nativeParams[i].type) {
        case VT_DOUBLE: nativeCall->setArgument(i, *nativeParams[i].xmm); break;
        default: nativeCall->setArgument(i, *nativeParams[i].gp); break;
        }
    }

    switch(retType) {
    case VT_DOUBLE:
        nativeCall->setReturn(*currentScope->function->retVar().xmm);
        break;
    case VT_VOID:
        break;
    default:
        nativeCall->setReturn(*currentScope->function->retVar().gp);
        break;
    }

    for(size_t i = 0; i < nativeParams.size(); ++i) {
        switch(nativeParams[i].type) {
        case VT_DOUBLE: c.unuse(*nativeParams[i].xmm); break;
        default: c.unuse(*nativeParams[i].gp); break;
        }
    }

}

void JITCompiler::visitReturnNode(ReturnNode *node) {
    currentNode = node;
    Compiler &c = currentCode();
    AsmJit::Label &retLabel = currentScope->function->retLabel();
    AsmJitVar &retVar = currentScope->function->retVar();

    if(node->returnExpr()) {
        node->returnExpr()->visit(this);
        convertTOSType(tosType(), currentScope->function->returnType());
    }
    switch(currentScope->function->returnType()) {
    case VT_INT: case VT_STRING: {
        AsmJitVar res = popCurrentStack();
        c.mov(*retVar.gp, *res.gp);
        c.jmp(retLabel);
        c.unuse(*res.gp);
        break;
    }
    case VT_DOUBLE: {
        AsmJitVar res = popCurrentStack();
        c.movq(*retVar.xmm, *res.xmm);
        c.jmp(retLabel);
        c.unuse(*res.xmm);
        break;
    }
    default:
        c.jmp(retLabel);
        break;
    }
}

//----------------------------------------------------------------------------------------------

VarType JITCompiler::numTypeCast(VarType ltype, VarType rtype, VarType toType, bool soft) {
    //if not soft then force cast to toType else skip cast if ltype == rtype
    if(!isNumType(ltype) || !isNumType(rtype)) throw std::logic_error("Numeric operation with NANs");
    if(ltype == rtype && soft) return ltype;

    if(ltype != toType) convertTOSType(ltype, toType);
    if(rtype != toType) {
        swapCurrentStack();
        convertTOSType(rtype, toType);
        swapCurrentStack();
    }

    return toType;
}

#define MATH_COMMUTATIVE_OP(c, t, iop, dop) {  \
    AsmJitVar a = popCurrentStack();        \
    AsmJitVar b = currentStack().top();     \
    if(t == VT_INT) {                       \
        c.iop(*b.gp, *a.gp);                \
        c.unuse(*a.gp);                     \
    } else {                                \
        c.dop(*b.xmm, *a.xmm);              \
        c.unuse(*a.xmm);                    \
    }                                       \
}

#define MATH_NON_COMMUTATIVE_OP(c, t, iop, dop) {  \
    AsmJitVar a = popCurrentStack();            \
    AsmJitVar b = popCurrentStack();            \
    if(t == VT_INT) {                           \
        c.iop(*a.gp, *b.gp);                    \
        c.unuse(*b.gp);                         \
    } else {                                    \
        c.dop(*a.xmm, *b.xmm);                  \
        c.unuse(*b.xmm);                        \
    }                                           \
    pushCurrentStack(a);                        \
}

#define MATH_CALL_INT_FUNCTION(c, func) {       \
    AsmJitVar a = popCurrentStack();            \
    AsmJitVar b = popCurrentStack();            \
    AsmJitVar res = createGPVar(VT_INT);        \
    ECall *call = c.call(imm((size_t)func));    \
    call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, int64_t, int64_t>()); \
    call->setArgument(0, *a.gp);                \
    call->setArgument(1, *b.gp);                \
    call->setReturn(*res.gp);                   \
    c.unuse(*a.gp);                             \
    c.unuse(*b.gp);                             \
    pushCurrentStack(res);                      \
}

#define MATH_CALL_DOUBLE_FUNCTION(c, func) {    \
    AsmJitVar a = popCurrentStack();            \
    AsmJitVar b = popCurrentStack();            \
    AsmJitVar res = createGPVar(VT_INT);        \
    ECall *call = c.call(imm((size_t)func));    \
    call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, double, double>()); \
    call->setArgument(0, *a.gp);                \
    call->setArgument(1, *b.gp);                \
    call->setReturn(*res.gp);                   \
    c.unuse(*a.gp);                             \
    c.unuse(*b.gp);                             \
    pushCurrentStack(res);                      \
}

static int64_t runIDIV(int64_t a, int64_t b) {
    return a / b;
}

static int64_t runIMOD(int64_t a, int64_t b) {
    return a % b;
}

void JITCompiler::makeMathOperation(TokenKind op, VarType ntype) {
    if(ntype != VT_INT && ntype != VT_DOUBLE) throw std::logic_error("Unsupported math operation type");

    Compiler &c = currentCode();
    switch(op) {
    case tADD: MATH_COMMUTATIVE_OP(c, ntype, add, addsd); break;
    case tMUL: MATH_COMMUTATIVE_OP(c, ntype, imul, mulsd); break;
    case tSUB: MATH_NON_COMMUTATIVE_OP(c, ntype, sub, subsd); break;

    //current AsmJit has some bug in idiv_lo_hi, so we use this workaround for div and mod
    case tDIV: {
        if(ntype == VT_INT) {
            MATH_CALL_INT_FUNCTION(c, runIDIV);
        } else {
            AsmJitVar a = popCurrentStack();
            AsmJitVar b = popCurrentStack();
            c.divsd(*a.xmm, *b.xmm);
            c.unuse(*b.xmm);
            pushCurrentStack(a);
        }
        break;
    }
    case tMOD:
        if(ntype != VT_INT) throw std::logic_error("mod operation can be done only with ints");
        MATH_CALL_INT_FUNCTION(c, runIMOD);
        break;
    default:
        throw std::logic_error(std::string("unsupported math operation: ") + tokenOp(op));
    }
}

//----------------------------------------------------------------------------------------------

bool JITCompiler::makeMathOperationWithVarReuse(BinaryOpNode *node) {
    AsmJitFunction *fn = currentScope->function;
    if(!analyzer[fn->id()]->isNodeWithMathReuse(node)) return false;

    Compiler &c = currentCode();
    if(node->left()->isIntLiteralNode() && node->right()->isLoadNode()) {
        currentScope->varReuseMode = true;
        c.mov(*fn->intLiteralBuffer().gp, node->left()->asIntLiteralNode()->literal());
        node->right()->visit(this); //got load var result on TOS
    } else if(node->right()->isIntLiteralNode() && node->left()->isLoadNode()) {
        currentScope->varReuseMode = true;
        c.mov(*fn->intLiteralBuffer().gp, node->right()->asIntLiteralNode()->literal());
        node->left()->visit(this);
    } else {
        return false;
    }

    AsmJitVar b = currentStack().top();
    if(node->kind() == tADD) c.add(*b.gp, *fn->intLiteralBuffer().gp);
    else c.imul(*b.gp, *fn->intLiteralBuffer().gp);
    return true;
}

//----------------------------------------------------------------------------------------------

//hack here: in comp function we have: 0 < (a < b) - inverse result of (a < b)
static int64_t compareDoubles(double a, double b) {
    if(a < b) return 1;
    if(a > b) return -1;
    return 0;
}

void JITCompiler::makeComparisonOperation(TokenKind op, VarType ntype) {
    Compiler &c = currentCode();
    if(ntype == VT_DOUBLE) {
        //use a little cheat to compare doubles
        MATH_CALL_DOUBLE_FUNCTION(c, compareDoubles);
        loadIntConst(0);
    }

    //at this point we have 2 ints on TOS
    AsmJitVar a = popCurrentStack();
    AsmJitVar b = popCurrentStack();
    c.cmp(*a.gp, *b.gp);

    AsmJit::Label setTrue = c.newLabel();
    AsmJit::Label setFalse = c.newLabel();

    switch(op) {
    case tEQ: c.je(setTrue); break;
    case tNEQ: c.jne(setTrue); break;
    case tLT: c.jl(setTrue); break;
    case tLE: c.jle(setTrue); break;
    case tGT: c.jg(setTrue); break;
    case tGE: c.jge(setTrue); break;
    default: throw std::logic_error(std::string("Unsupported comparison operation: ") + tokenStr(op));
    }

    c.unuse(*a.gp);
    c.unuse(*b.gp);

    loadIntConst(0);
    c.jmp(setFalse);
    c.bind(setTrue);
    loadIntConst(1, true);
    c.bind(setFalse);
}

//----------------------------------------------------------------------------------------------

void JITCompiler::makeBitwiseOperation(TokenKind op, VarType ntype) {
    if(ntype != VT_INT) throw std::logic_error("Bitwise operation with non-ints");
    Compiler &c = currentCode();

    AsmJitVar a = popCurrentStack();
    AsmJitVar b = popCurrentStack();

    switch(op) {
    case tAAND: c.and_(*a.gp, *b.gp); break;
    case tAOR: c.or_(*a.gp, *b.gp); break;
    case tAXOR: c.xor_(*a.gp, *b.gp); break;
    default: throw std::logic_error(std::string("Unsupported bitwise operation: ") + tokenStr(op));
    }

    c.unuse(*b.gp);
    pushCurrentStack(a);
}

//----------------------------------------------------------------------------------------------

void JITCompiler::makeLazyLogicOperation(BinaryOpNode *node) {
    Compiler &c = currentCode();

    AsmJit::Label setTrue = c.newLabel();
    AsmJit::Label setFalse = c.newLabel();
    AsmJit::Label exitLabel = c.newLabel();

    node->left()->visit(this);
    convertTOSType(tosType(), VT_INT);

    switch(node->kind()) {
    case tAND: {
        AsmJitVar a = popCurrentStack();
        c.cmp(*a.gp, imm(0));
        c.je(setFalse);  //if left is 0 - goto set 0 and exit
        c.unuse(*a.gp);
        //left arg is 1
        break;
    }
    case tOR: {
        AsmJitVar a = popCurrentStack();
        c.cmp(*a.gp, imm(0));
        c.jne(setTrue);  //if left is not 0 - goto set 1 and exit
        c.unuse(*a.gp);
        //left arg is 0
        break;
    }
    default:
        throw std::logic_error("unsupprted logic operation");
    }

    node->right()->visit(this);
    convertTOSType(tosType(), VT_INT);
    AsmJitVar a = popCurrentStack();
    c.cmp(*a.gp, imm(0));
    c.jne(setTrue); //if right is not 0 - goto set 1 and exit
    c.unuse(*a.gp);
    c.bind(setFalse);
    loadIntConst(0);
    c.jmp(exitLabel);
    c.bind(setTrue);
    loadIntConst(1, true);
    c.bind(exitLabel);
}

//----------------------------------------------------------------------------------------------

void JITCompiler::visitBinaryOpNode(BinaryOpNode *node) {
    currentNode = node;
    FunctionDescription *fd = analyzer[currentScope->function->id()];

    if(node->kind() == tOR || node->kind() == tAND) {
        makeLazyLogicOperation(node);
        return;
    }

    if(fd->canReuseMathVars && makeMathOperationWithVarReuse(node)) return;

    node->right()->visit(this); // upper (left) op lower (right)
    VarType rtype = tosType();
    node->left()->visit(this);
    VarType ltype = tosType();

    switch(node->kind()) {
    case tEQ: case tNEQ: case tGE: case tGT: case tLE: case tLT: {
        //if one of operands is double, then cast another to double
        VarType ntype = numTypeCast(ltype, rtype, VT_DOUBLE, true);
        makeComparisonOperation(node->kind(), ntype);
        break;
    }
    case tADD: case tSUB: case tMUL: case tDIV: case tMOD: {
        VarType ntype = numTypeCast(ltype, rtype, (node->kind() == tMOD ? VT_INT : VT_DOUBLE), true);
        makeMathOperation(node->kind(), ntype);
        break;
    }
    case tAOR: case tAXOR: case tAAND: {
        //force cast to int
        VarType ntype = numTypeCast(ltype, rtype, VT_INT, false);
        makeBitwiseOperation(node->kind(), ntype);
        break;
    }
    default: throw std::logic_error("unsupported binary operation");
    }
}

void JITCompiler::visitUnaryOpNode(UnaryOpNode *node) {
    currentNode = node;
    Compiler &c = currentCode();

    node->operand()->visit(this);
    switch(node->kind()) {
    case tNOT: {
        convertTOSType(tosType(), VT_INT);

        AsmJit::Label setFalse = c.newLabel();
        AsmJit::Label setTrue = c.newLabel();

        AsmJitVar var = popCurrentStack();
        c.cmp(*var.gp, imm(0)); //if current val is 0 - goto set 1, else set 0 and exit
        c.je(setTrue);
        c.unuse(*var.gp);
        loadIntConst(0);
        c.jmp(setFalse);
        c.bind(setTrue);
        loadIntConst(1, true);
        c.bind(setFalse);

        break;
    }
    case tSUB: {
        VarType tos = tosType();
        if(!isNumType(tos)) throw std::logic_error("Numeric operation with NANs");
        if(tos == VT_INT) {
            currentCode().neg(*currentStack().top().gp); break;
        } else {
            AsmJitVar res = createXMMVar(0.0);
            AsmJitVar arg = popCurrentStack();
            c.subsd(*res.xmm, *arg.xmm);
            c.unuse(*arg.xmm);
            pushCurrentStack(res);
        }
        break;
    }
    default:
        throw std::logic_error(std::string("Unsupported unary operation: ") + tokenOp(node->kind()));
    }
}

//----------------------------------------------------------------------------------------------

void JITCompiler::visitIfNode(IfNode *node) {
    currentNode = node;
    Compiler &c = currentCode();

    if(node->elseBlock()) {
        AsmJit::Label ifElse = c.newLabel();

        //duct tape for AsmJit labels
        IfChecker ic;
        bool noRetInThen = !ic.hasReturn(node->thenBlock());
        AsmJit::Label ifEnd;
        if(noRetInThen) ifEnd = c.newLabel();

        node->ifExpr()->visit(this);
        convertTOSType(tosType(), VT_INT);
        AsmJitVar cond = popCurrentStack();
        c.cmp(*cond.gp, imm(0));
        c.je(ifElse);
        c.unuse(*cond.gp);
        node->thenBlock()->visit(this);
        if(noRetInThen) c.jmp(ifEnd);
//        c.jmp(ifEnd);
        c.bind(ifElse);
        node->elseBlock()->visit(this);
        if(noRetInThen) c.bind(ifEnd);
//        c.bind(ifEnd);
    } else {
        AsmJit::Label ifEnd = c.newLabel();

        node->ifExpr()->visit(this);
        convertTOSType(tosType(), VT_INT);
        AsmJitVar cond = popCurrentStack();
        c.cmp(*cond.gp, imm(0));
        c.je(ifEnd);
        c.unuse(*cond.gp);
        node->thenBlock()->visit(this);
        c.bind(ifEnd);
    }
}

void JITCompiler::visitForNode(ForNode *node) {
    currentNode = node;
    Compiler &c = currentCode();
    AsmJit::Label forStart = c.newLabel();
    AsmJit::Label forEnd = c.newLabel();

    if(node->var()->type() != VT_INT) throw std::logic_error("Non int var in for");
    if(!node->inExpr()->isBinaryOpNode()) throw std::logic_error("Non range operation in for");
    BinaryOpNode *inExpr = node->inExpr()->asBinaryOpNode();
    if(inExpr->kind() != tRANGE) throw std::logic_error("Non range operation in for");

    inExpr->left()->visit(this);
    if(tosType() != VT_INT) throw std::logic_error("Non int range in for");
    storeVar(node->var());

    c.bind(forStart);
    inExpr->right()->visit(this);
    if(tosType() != VT_INT) throw std::logic_error("Non int range in for");
    loadVar(node->var());

    AsmJitVar forVar = popCurrentStack();
    AsmJitVar forRange = popCurrentStack();
    c.cmp(*forVar.gp, *forRange.gp);
    c.jg(forEnd);
    c.unuse(*forVar.gp);
    c.unuse(*forRange.gp);

    node->body()->visit(this);

    loadVar(node->var());
    forVar = currentStack().top();
    c.inc(*forVar.gp);
    storeVar(node->var());
    c.unuse(*forVar.gp);
    c.jmp(forStart);
    c.bind(forEnd);
}

void JITCompiler::visitWhileNode(WhileNode *node) {
    currentNode = node;
    Compiler &c = currentCode();
    AsmJit::Label whileStart = c.newLabel();
    AsmJit::Label whileEnd = c.newLabel();

    c.bind(whileStart);
    node->whileExpr()->visit(this);
    convertTOSType(tosType(), VT_INT);
    AsmJitVar cond = popCurrentStack();
    c.cmp(*cond.gp, imm(0));
    c.je(whileEnd);
    c.unuse(*cond.gp);
    node->loopBlock()->visit(this);
    c.jmp(whileStart);
    c.bind(whileEnd);
}

//----------------------------------------------------------------------------------------------

void JITCompiler::visitIntLiteralNode(IntLiteralNode *node) {
    if(currentScope->inPrintNode) {
        AsmJitVar &printBuffer = currentScope->function->intPrintBuffer();
        currentCode().mov(*printBuffer.gp, imm(node->literal()));
        printBuffer.type = VT_INT;
        pushCurrentStack(printBuffer);
    } else {
        pushCurrentStack(createGPVar(VT_INT, node->literal()));
    }
}

void JITCompiler::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    pushCurrentStack(createXMMVar(node->literal()));
}

void JITCompiler::visitStringLiteralNode(StringLiteralNode *node) {
    uint16_t id = code->makeStringConstant(node->literal());
    const char *str = code->constantById(id).c_str();
    if(currentScope->inPrintNode) {
        AsmJitVar &printBuffer = currentScope->function->intPrintBuffer();
        currentCode().mov(*printBuffer.gp, imm((int64_t)str));
        printBuffer.type = VT_STRING;
        pushCurrentStack(printBuffer);
    } else {
        pushCurrentStack(createGPVar(VT_STRING, (int64_t)str));
    }
}

void JITCompiler::visitStoreNode(StoreNode *node) {
    currentNode = node;
    node->value()->visit(this);

    switch(node->op()) {
    case tASSIGN: break;
    case tINCRSET: case tDECRSET: {
        VarType rtype = tosType();
        loadVar(node->var());
        VarType ltype = tosType();
        VarType ntype = numTypeCast(ltype, rtype, ltype, false);
        makeMathOperation(node->op() == tINCRSET ? tADD : tSUB, ntype);
        break;
    }
    default:
        throw std::logic_error("Unsupported operation in StoreNode");
    }

    storeVar(node->var());
}

void JITCompiler::visitLoadNode(LoadNode *node) {
    currentNode = node;
    loadVar(node->var());
}

//----------------------------------------------------------------------------------------------

static void printDouble(double val) {
    printf("%g", val);
}

static void printInt(int64_t val) {
    printf("%ld", val);
}

static void printString(char *str) {
    printf("%s", str);
}

void JITCompiler::visitPrintNode(PrintNode *node) {
    currentNode = node;
    Compiler &c = currentCode();
    FunctionDescription *fd = analyzer[currentScope->function->id()];

    currentScope->inPrintNode = false;
    for(uint32_t i = 0; i < node->operands(); i++) {
        AstNode *nnode = node->operandAt(i);
        if(fd->hasIntPrintNode) currentScope->inPrintNode = nnode->isLoadNode() || nnode->isStringLiteralNode() || nnode->isIntLiteralNode();
        nnode->visit(this);

        VarType tos = tosType();
        AsmJitVar vPrint = popCurrentStack();
        ECall* printCall = 0;
        switch(tos) {
        case VT_DOUBLE:
            printCall = c.call(imm((size_t)printDouble));
            printCall->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, double>());
            printCall->setArgument(0, *vPrint.xmm);
            c.unuse(*vPrint.xmm);
            break;
        case VT_INT:
            printCall = c.call(imm((size_t)printInt));
            printCall->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int64_t>());
            printCall->setArgument(0, *vPrint.gp);
            if(!currentScope->inPrintNode) c.unuse(*vPrint.gp);
            break;
        case VT_STRING:
            printCall = c.call(imm((size_t)printString));
            printCall->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int64_t>());
            printCall->setArgument(0, *vPrint.gp);
            if(!currentScope->inPrintNode) c.unuse(*vPrint.gp);
            break;
        default:
            throw std::logic_error("print called with unsupported type");
        }
    }
    currentScope->inPrintNode = false;
}
