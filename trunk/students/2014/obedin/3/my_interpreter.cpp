#include "my_interpreter.hpp"

#include "asmjit/asmjit.h"


template<typename T>
StackItem
mkStackItem(T val);

template<>
StackItem
mkStackItem<int64_t>(int64_t val)
{ return StackItem(VT_INT, val); }

template<>
StackItem
mkStackItem<double>(double val)
{ return StackItem(VT_DOUBLE, 0, val); }

template<>
StackItem
mkStackItem<const char *>(const char *val)
{ return StackItem(VT_STRING, 0, 0.0, val); }


#define GEN_AS_FOR(vt, m) \
    if (type != (vt)) \
        throw std::runtime_error(MSG_UNEXPECTED_TOS);\
    return (m);

template<>
int64_t
StackItem::as<int64_t>()
{ GEN_AS_FOR(VT_INT, m_i) }

template<>
double
StackItem::as<double>()
{ GEN_AS_FOR(VT_DOUBLE, m_d) }

template<>
const char*
StackItem::as<const char*>()
{ GEN_AS_FOR(VT_STRING, m_s) }

#undef GEN_AS_FOR


template<typename T>
T
ICode::next()
{
    T val = bc()->getTyped<T>(ip());
    ip() += sizeof(T);
    return val;
}

template<>
const char*
ICode::next<const char*>()
{
    return constantById(next<uint16_t>()).c_str();
}



Status*
ICode::execute(std::vector<Var *> &vars)
{
#ifdef DUMP_BYTECODE
    disassemble();
#endif

    IScope topScope((BytecodeFunction *) functionById(0));
    m_curScope = &topScope;

    static void* dispatchTable[] = {
#define HANDLE_ELEM(b, d, l) &&h##b,
    FOR_BYTECODES(HANDLE_ELEM)
#undef HANDLE_ELEM
    };

#define DISPATCH goto *dispatchTable[bc()->getInsn(ip()++)];

    DISPATCH
    while (true) {
hINVALID:
hS2I:
        return Status::Error(MSG_INVALID_INSN);
hDLOAD:
        stackPush(mkStackItem(next<double>()));
        DISPATCH
hILOAD:
        stackPush(mkStackItem(next<int64_t>()));
        DISPATCH
hSLOAD:
        stackPush(mkStackItem<const char*>(next<const char*>()));
        DISPATCH
hDLOAD0:
        stackPush(mkStackItem<double>(0.0));
        DISPATCH
hILOAD0:
        stackPush(mkStackItem<int64_t>(0));
        DISPATCH
hSLOAD0:
        stackPush(mkStackItem<const char*>(""));
        DISPATCH
hDLOAD1:
        stackPush(mkStackItem<double>(1.0));
        DISPATCH
hILOAD1:
        stackPush(mkStackItem<int64_t>(1));
        DISPATCH
hDLOADM1:
        stackPush(mkStackItem<double>(-1.0));
        DISPATCH
hILOADM1:
        stackPush(mkStackItem<int64_t>(-1));
        DISPATCH

hDADD:
        doNumeric<double>(tADD);
        DISPATCH
hDSUB:
        doNumeric<double>(tSUB);
        DISPATCH
hDMUL:
        doNumeric<double>(tMUL);
        DISPATCH
hDDIV:
        doNumeric<double>(tDIV);
        DISPATCH
hIADD:
        doNumeric<int64_t>(tADD);
        DISPATCH
hISUB:
        doNumeric<int64_t>(tSUB);
        DISPATCH
hIMUL:
        doNumeric<int64_t>(tMUL);
        DISPATCH
hIDIV:
        doNumeric<int64_t>(tDIV);
        DISPATCH

hDNEG:
        stackPush(mkStackItem(-(stackPop().as<double>())));
        DISPATCH
hINEG:
        stackPush(mkStackItem(-(stackPop().as<int64_t>())));
        DISPATCH

hIPRINT:
        std::cout << stackPop().as<int64_t>();
        DISPATCH
hDPRINT:
        std::cout << stackPop().as<double>();
        DISPATCH
hSPRINT:
        std::cout << stackPop().as<const char*>();
        DISPATCH

hIMOD:
        doBitwise(tMOD);
        DISPATCH
hIAOR:
        doBitwise(tAOR);
        DISPATCH
hIAAND:
        doBitwise(tAAND);
        DISPATCH
hIAXOR:
        doBitwise(tAXOR);
        DISPATCH

hI2D:
        stackPush(mkStackItem<double>(stackPop().as<int64_t>()));
        DISPATCH
hD2I:
        stackPush(mkStackItem<int64_t>(stackPop().as<double>()));
        DISPATCH

hDCMP:
        doComparison<double>();
        DISPATCH
hICMP:
        doComparison<int64_t>();
        DISPATCH

hSWAP:
        {
            StackItem a = stackPop();
            StackItem b = stackPop();
            stackPush(a);
            stackPush(b);
        }
        DISPATCH
hPOP:
        stackPop();
        DISPATCH

hJA:
        ip() += next<int16_t>() - sizeof(int16_t);
        DISPATCH
hIFICMPNE:
        doCmpAndGo(tNEQ);
        DISPATCH
hIFICMPE:
        doCmpAndGo(tEQ);
        DISPATCH
hIFICMPG:
        doCmpAndGo(tGT);
        DISPATCH
hIFICMPGE:
        doCmpAndGo(tGE);
        DISPATCH
hIFICMPL:
        doCmpAndGo(tLT);
        DISPATCH
hIFICMPLE:
        doCmpAndGo(tLE);
        DISPATCH

hLOADDVAR0:
hLOADIVAR0:
hLOADSVAR0:
        stackPush(localVar(0));
        DISPATCH

hLOADDVAR1:
hLOADIVAR1:
hLOADSVAR1:
        stackPush(localVar(1));
        DISPATCH

hLOADDVAR2:
hLOADIVAR2:
hLOADSVAR2:
        stackPush(localVar(2));
        DISPATCH

hLOADDVAR3:
hLOADIVAR3:
hLOADSVAR3:
        stackPush(localVar(3));
        DISPATCH

hSTOREDVAR0:
hSTOREIVAR0:
hSTORESVAR0:
        localVar(0) = stackPop();
        DISPATCH

hSTOREDVAR1:
hSTOREIVAR1:
hSTORESVAR1:
        localVar(1) = stackPop();
        DISPATCH

hSTOREDVAR2:
hSTOREIVAR2:
hSTORESVAR2:
        localVar(2) = stackPop();
        DISPATCH

hSTOREDVAR3:
hSTOREIVAR3:
hSTORESVAR3:
        localVar(3) = stackPop();
        DISPATCH

hLOADDVAR:
hLOADIVAR:
hLOADSVAR:
        stackPush(localVar(next<uint16_t>()));
        DISPATCH

hSTOREDVAR:
hSTOREIVAR:
hSTORESVAR:
        localVar(next<uint16_t>()) = stackPop();
        DISPATCH

hLOADCTXDVAR:
hLOADCTXIVAR:
hLOADCTXSVAR:
        {
            ID ctx = next<uint16_t>();
            ID id  = next<uint16_t>();
            stackPush(ctxVar(ctx, id));
        }
        DISPATCH

hSTORECTXDVAR:
hSTORECTXIVAR:
hSTORECTXSVAR:
        {
            ID ctx = next<uint16_t>();
            ID id  = next<uint16_t>();
            ctxVar(ctx, id) = stackPop();
        }
        DISPATCH

hCALL:
        doCallFunction(next<uint16_t>());
        DISPATCH
hCALLNATIVE:
        doCallNativeFunction(next<uint16_t>());
        DISPATCH
hRETURN:
        {
            IScope *scope = m_curScope;
            m_curScope = scope->parent;
            delete scope;
        }
        DISPATCH

hBREAK:
hDUMP:
hSTOP:
        break;
    }

#undef DISPATCH

    return Status::Ok();
}


template<typename T>
void
ICode::doNumeric(TokenKind op)
{
    T a = stackPop().as<T>();
    T b = stackPop().as<T>();

    switch (op) {
        case tADD:
            stackPush(mkStackItem(a + b)); break;
        case tSUB:
            stackPush(mkStackItem(a - b)); break;
        case tMUL:
            stackPush(mkStackItem(a * b)); break;
        case tDIV:
            stackPush(mkStackItem(a / b)); break;
        default: break;
    }
}

template<typename T>
void
ICode::doComparison()
{
    T a = stackPop().as<T>();
    T b = stackPop().as<T>();

    int64_t result = a == b ? 0 : a < b ? -1 : 1;
    stackPush(mkStackItem(result));
}

void
ICode::doBitwise(TokenKind op)
{
    int64_t a = stackPop().as<int64_t>();
    int64_t b = stackPop().as<int64_t>();

    switch (op) {
        case tMOD:
            stackPush(mkStackItem(a % b)); break;
        case tAOR:
            stackPush(mkStackItem(a | b)); break;
        case tAAND:
            stackPush(mkStackItem(a & b)); break;
        case tAXOR:
            stackPush(mkStackItem(a ^ b)); break;
        default: break;
    }
}

void
ICode::doCmpAndGo(TokenKind op)
{
    int64_t a = stackPop().as<int64_t>();
    int64_t b = stackPop().as<int64_t>();
    int16_t offset = next<int16_t>() - sizeof(int16_t);

    switch (op) {
        case tEQ:
            if (a == b) ip() += offset; break;
        case tNEQ:
            if (a != b) ip() += offset; break;
        case tGT:
            if (a > b)  ip() += offset; break;
        case tGE:
            if (a >= b) ip() += offset; break;
        case tLT:
            if (a < b)  ip() += offset; break;
        case tLE:
            if (a <= b) ip() += offset; break;
        default: break;
    }
}

void
ICode::doCallFunction(ID id)
{
    BytecodeFunction *fn = (BytecodeFunction*)functionById(id);
    if (fn == NULL)
        throw std::runtime_error(MSG_FUNCTION_NOT_FOUND);
    m_curScope = new IScope(fn, m_curScope);
}

uint32_t
varTypeToAsmJit(VarType t)
{
    switch (t) {
        case VT_DOUBLE:
            return asmjit::kX86VarTypeXmmSd;
        case VT_INT:
        case VT_STRING:
            return asmjit::kVarTypeInt64;
        default:
            throw std::runtime_error(MSG_CANT_CONVERT_TO_ASMJIT_TYPE);
    }
}

void
ICode::doCallNativeFunction(ID id)
{
    const Signature *sig;
    const string *name;
    const void *addr = nativeById(id, &sig, &name);
    if (addr == NULL)
        throw std::runtime_error(MSG_NATIVE_NOT_FOUND);
    const VarType returnType = sig->at(0).first;

    using namespace asmjit;

    JitRuntime runtime;
    X86Compiler compiler(&runtime);

    FuncBuilderX nativeFnBuilder, mainFnBuilder;
    if (returnType != VT_VOID) {
        nativeFnBuilder.setRet(varTypeToAsmJit(returnType));
        mainFnBuilder.setRet(varTypeToAsmJit(returnType));
    }
    compiler.addFunc(kFuncConvHost, mainFnBuilder);

    X86GpVar nativeFnPtr(compiler, kVarTypeIntPtr);
    compiler.mov(nativeFnPtr, imm_ptr((void *)addr));

    vector<asmjit::Var> args;
    for (uint16_t i = 1; i < sig->size(); ++i) {
        uint32_t varType = varTypeToAsmJit(sig->at(i).first);
        nativeFnBuilder.addArg(varType);

        switch (sig->at(i).first) {
            case VT_DOUBLE: {
                X86XmmVar arg(compiler, varType);
                X86GpVar tmp = compiler.newGpVar();
                double val = localVar(i-1).as<double>();
                compiler.mov(tmp, *(reinterpret_cast<uint64_t *>(&val)));
                compiler.movq(arg, tmp.m());
                compiler.unuse(tmp);
                args.push_back(arg);
                break;
            }
            case VT_INT: {
                X86GpVar arg(compiler, varType);
                compiler.mov(arg, localVar(i-1).as<int64_t>());
                args.push_back(arg);
                break;
            }
            case VT_STRING: {
                X86GpVar arg(compiler, varType);
                compiler.mov(arg, (int64_t)localVar(i-1).as<const char *>());
                args.push_back(arg);
                break;
            }
            default:
                throw std::runtime_error(MSG_CANT_CONVERT_TO_ASMJIT_TYPE);
        }
    }

    X86CallNode *nativeFnCall = compiler.call(nativeFnPtr, kFuncConvHost, nativeFnBuilder);
    for (size_t i = 0; i < args.size(); ++i)
        nativeFnCall->setArg(i, args[i]);

    switch (returnType) {
        case VT_VOID:
            compiler.ret();
            break;
        case VT_DOUBLE: {
            X86XmmVar rvar(compiler, varTypeToAsmJit(returnType));
            nativeFnCall->setRet(0, rvar);
            compiler.ret(rvar);
            break;
        }
        case VT_INT:
        case VT_STRING: {
            X86GpVar rvar(compiler, varTypeToAsmJit(returnType));
            nativeFnCall->setRet(0, rvar);
            compiler.ret(rvar);
            break;
        }
        default:
            throw std::runtime_error(MSG_CANT_CONVERT_TO_ASMJIT_TYPE);
    }

    compiler.endFunc();
    void *mainFnPtr = compiler.make();
    switch (returnType) {
        case VT_VOID:
            asmjit_cast<void (*)()>(mainFnPtr)();
            break;
        case VT_DOUBLE: {
            double result = asmjit_cast<double (*)()>(mainFnPtr)();
            stackPush(mkStackItem<double>(result));
            break;
        }
        case VT_INT: {
            int64_t result = asmjit_cast<int64_t (*)()>(mainFnPtr)();
            stackPush(mkStackItem<int64_t>(result));
            break;
        }
        case VT_STRING: {
            const char *result = asmjit_cast<const char * (*)()>(mainFnPtr)();
            stackPush(mkStackItem<const char*>(result));
            break;
        }
        default:
            throw std::runtime_error(MSG_CANT_CONVERT_TO_ASMJIT_TYPE);
    }

    runtime.release(mainFnPtr);
}
