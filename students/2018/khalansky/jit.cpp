#include "jit.hpp"
#include <unordered_map>
#include <stack>
#include <cstdio>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbool-operation"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#include <asmjit/asmjit.h>
#pragma GCC diagnostic pop

using namespace asmjit;

namespace mathvm {

void iprint(int64_t i) {
    cout << i;
}

void dprint(double d) {
    cout << d;
}

void sprint(const char *s) {
    string s2(s);
    cout << s2;
}

int64_t dcmp(double a, double b) {
    if (a < b) {
        return -1;
    } else if (a > b) {
        return 1;
    } else {
        return 0;
    }
}

static uint8_t varTypeToJitType(VarType type)
{
    switch (type) {
        case VT_DOUBLE:
            return TypeIdOf<double>::kTypeId;
            break;
        case VT_INT:
            return TypeIdOf<int64_t>::kTypeId;
            break;
        case VT_STRING:
            return TypeIdOf<const char*>::kTypeId;
            break;
        case VT_VOID:
            return TypeIdOf<void>::kTypeId;
            break;
        default:
            assert(0);
            return 0;
    }
}

class JitEnvImpl {

    FileLogger log;
    InterpreterCodeImpl *bytecodeCode;

    JitRuntime rt;

    size_t maxFnId;
    void **funcTable;
    size_t entryPoint;
    vector<vector<uint8_t> > args;
    vector<FuncSignature> sigs;

    void *translateFunction(
        const BytecodeFunction &function, CodeHolder& code);

public:

    void execute();

    JitEnvImpl(InterpreterCodeImpl&);

    ~JitEnvImpl();


};

void JitEnvImpl::execute()
{
    ((void (*)())funcTable[entryPoint])();
}

void JitEnvironment::execute()
{
    inner->execute();
}

uint8_t insn_sizes(Instruction instr);

const char *insn_names(Instruction instr);

JitEnvImpl::JitEnvImpl(InterpreterCodeImpl& bytecode):
log(stderr), bytecodeCode(&bytecode)
{

    {
        maxFnId = 0;
        Code::FunctionIterator fnit(
            const_cast<InterpreterCodeImpl*>(&bytecode));
        while (fnit.hasNext()) {
            auto *fn = fnit.next();
            maxFnId = max(maxFnId, (size_t)(fn->id() + 1));
        }
        funcTable = new void*[maxFnId];
        args.resize(maxFnId);
        sigs.resize(maxFnId);
    }

    {
        Code::FunctionIterator fnit(&bytecode);
        while (fnit.hasNext()) {
            auto *bfn = dynamic_cast<BytecodeFunction*>(fnit.next());
            size_t id = bfn->id();
            args[id].resize(bfn->parametersNumber());
            for (uint16_t i = 0; i < bfn->parametersNumber(); ++i) {
                args[id][i] = varTypeToJitType(bfn->parameterType(i));
            }
            sigs[id].init(
                CallConv::kIdHost,
                varTypeToJitType(bfn->returnType()),
                args[id].data(),
                bfn->parametersNumber());
        }
    }

    {
        Code::FunctionIterator fnit(&bytecode);
        while (fnit.hasNext()) {
            CodeHolder code;
            code.init(rt.getCodeInfo());
            code.setLogger(&log);
            auto *bfn = dynamic_cast<BytecodeFunction*>(fnit.next());
            assert(bfn);
            funcTable[bfn->id()] = translateFunction(*bfn, code);
        }
    }

    {
        const TranslatedFunction *top = bytecode.functionByName("<top>");
        entryPoint = top->id();
    }

}

JitEnvImpl::~JitEnvImpl() {
    delete bytecodeCode;
    delete[] funcTable;
}

void * JitEnvImpl::translateFunction(const BytecodeFunction &function,
    CodeHolder& code)
{
    cerr << "Translating function " << function.name() << endl;

    const uint8_t dcmpArgTypes[] = {
        varTypeToJitType(VT_DOUBLE), varTypeToJitType(VT_DOUBLE) };

    FuncSignature dcmpSign;
    dcmpSign.init(CallConv::kIdHost, varTypeToJitType(VT_INT), dcmpArgTypes, 2);


    const Bytecode *bytecode = (
        const_cast<BytecodeFunction*>(&function))->bytecode();
    unordered_map<uint32_t, asmjit::Label> unboundLabels;
    unordered_map<uint32_t, asmjit::Label> jumpFrom;

    X86Compiler cc(&code);

    vector<X86Gp> vars(function.localsNumber());
    for (auto &var : vars) {
        var = cc.newI64();
    }
    vector<X86Xmm> dvars(function.localsNumber());
    for (auto &var : dvars) {
        var = cc.newXmm();
    }
    stack<X86Gp> gpRegs;
    stack<X86Xmm> xmmRegs;

    asmjit::Label funcEnd = cc.newLabel();

    X86Xmm d1;
    X86Xmm d2;
    X86Gp i1;
    X86Gp i2;
    X86Gp i3;
    X86Mem m1;
    X86Mem m2;
    cc.addFunc(sigs[function.id()]);

    for (size_t j = 0; j < bytecode->length(); ++j) {
        cerr << j << " ";
        Instruction instr = (Instruction)bytecode->get(j);
        cerr << insn_names(instr);
        for (size_t i = 1; i < insn_sizes(instr); ++i) {
            cerr << " " << (int)(bytecode->get(j+i));
        }
        j += insn_sizes(instr)-1;
        cerr << endl;
    }
    cerr << "END OF PROGRAM" << endl;

    for (uint16_t i = 0; i < function.parametersNumber(); ++i) {
        switch (function.parameterType(i)) {
            case VT_DOUBLE: {
                X86Xmm p = cc.newXmm();
                cc.setArg(i, p);
                xmmRegs.push(p);
                break;
            }
            case VT_INT:
            case VT_STRING: {
                X86Gp p = cc.newI64();
                cc.setArg(i, p);
                gpRegs.push(p);
                break;
            }
            case VT_INVALID:
            case VT_VOID:
                assert(0);
                break;
        }
    }

    Instruction instr; size_t idx;
    for (idx = 0, instr = (Instruction)bytecode->get(idx);
        idx < bytecode->length();
        idx += insn_sizes(instr),
        idx < bytecode->length() && (instr = (Instruction)bytecode->get(idx)))
    {
        uint32_t lb;
        switch (instr) {
            case BC_JA:
            case BC_IFICMPNE:
            case BC_IFICMPE:
            case BC_IFICMPG:
            case BC_IFICMPGE:
            case BC_IFICMPL:
            case BC_IFICMPLE:
                lb = idx+1 + bytecode->getInt16(idx+1);
                if (!unboundLabels.count(lb)) {
                    unboundLabels[lb] = cc.newLabel();
                }
                jumpFrom[idx] = unboundLabels[lb];
                break;
            default:
                break;
        }
    }

    for (idx = 0, instr = (Instruction)bytecode->get(idx);
        idx < bytecode->length();
        idx += insn_sizes(instr),
        idx < bytecode->length() && (instr = (Instruction)bytecode->get(idx)))
    {
#define BIND_LABELS if (unboundLabels.count(idx)) { \
            cc.bind(unboundLabels[idx]); \
        }
        BIND_LABELS

        cerr << gpRegs.size() << " " << xmmRegs.size() << " " << idx << " " << insn_names(instr);
        for (size_t i = 1; i < insn_sizes(instr); ++i) {
            cerr << " " << (int)(bytecode->get(idx+i));
        }
        cerr << endl;
        switch (instr) {
            case BC_INVALID:
                assert(0);
                break;
            case BC_DLOAD:
                d1 = cc.newXmm();
                m1 = cc.newDoubleConst(kConstScopeLocal,
                    bytecode->getDouble(idx+1));
                cc.movsd(d1, m1);
                xmmRegs.push(d1);
                break;
            case BC_ILOAD:
                i1 = cc.newI64();
                cc.mov(i1, bytecode->getInt64(idx+1));
                gpRegs.push(i1);
                break;
            case BC_SLOAD:
                i1 = cc.newUIntPtr();
                cc.mov(i1, imm_ptr(
                    bytecodeCode->constantById(bytecode->getInt16(idx+1)).c_str()));
                gpRegs.push(i1);
                break;
            case BC_DLOAD0:
                d1 = cc.newXmm();
                m1 = cc.newDoubleConst(kConstScopeLocal, 0.0);
                cc.movsd(d1, m1);
                xmmRegs.push(d1);
                break;
            case BC_ILOAD0:
                i1 = cc.newI64();
                cc.xor_(i1, i1);
                if ((Instruction)bytecode->get(idx+insn_sizes(instr)) == BC_JA) {
                    idx += insn_sizes(BC_ILOAD0);
                    cc.jmp(jumpFrom[idx]);
                    idx += insn_sizes(BC_JA);
                    BIND_LABELS
                    cc.mov(i1, 1);
                }
                gpRegs.push(i1);
                break;
            case BC_SLOAD0:
                i1 = cc.newUIntPtr();
                cc.mov(i1, imm_ptr(""));
                gpRegs.push(i1);
                break;
            case BC_DLOAD1:
                d1 = cc.newXmm();
                m1 = cc.newDoubleConst(kConstScopeLocal, 1.0);
                cc.movsd(d1, m1);
                xmmRegs.push(d1);
                break;
            case BC_ILOAD1:
                i1 = cc.newI64();
                cc.mov(i1, 1);
                if ((Instruction)bytecode->get(idx+insn_sizes(instr)) == BC_JA) {
                    idx += insn_sizes(BC_ILOAD1);
                    cc.jmp(jumpFrom[idx]);
                    idx += insn_sizes(BC_JA);
                    BIND_LABELS
                    cc.xor_(i1, i1);
                }
                gpRegs.push(i1);
                break;
            case BC_DLOADM1:
                d1 = cc.newXmm();
                m1 = cc.newDoubleConst(kConstScopeLocal, -1.0);
                cc.movsd(d1, m1);
                xmmRegs.push(d1);
                break;
            case BC_ILOADM1:
                i1 = cc.newI64();
                cc.mov(i1, -1);
                gpRegs.push(i1);
                break;
#define XmmBinOp(op) \
                d1 = xmmRegs.top(); \
                xmmRegs.pop(); \
                d2 = xmmRegs.top(); \
                xmmRegs.pop(); \
                cc.op(d1, d2); \
                xmmRegs.push(d1);
            case BC_DADD:
                XmmBinOp(addsd);
                break;
            case BC_IADD:
                i1 = (gpRegs.top());
                gpRegs.pop();
                i2 = (gpRegs.top());
                gpRegs.pop();
                cc.add(i1, i2);
                gpRegs.push(i1);
                break;
            case BC_DSUB:
                XmmBinOp(subsd);
                break;
            case BC_ISUB:
                i1 = (gpRegs.top());
                gpRegs.pop();
                i2 = (gpRegs.top());
                gpRegs.pop();
                cc.sub(i1, i2);
                gpRegs.push(i1);
                break;
            case BC_DMUL:
                XmmBinOp(mulsd);
                break;
            case BC_IMUL:
                i1 = (gpRegs.top());
                gpRegs.pop();
                i2 = (gpRegs.top());
                gpRegs.pop();
                cc.imul(i1, i2);
                gpRegs.push(i1);
                break;
            case BC_DDIV:
                XmmBinOp(divsd);
                break;
            case BC_IDIV:
                i1 = (gpRegs.top());
                gpRegs.pop();
                i2 = (gpRegs.top());
                gpRegs.pop();
                i3 = cc.newI64();
                cc.xor_(i3, i3);
                cc.idiv(i3, i1, i2);
                gpRegs.push(i1);
                break;
            case BC_IMOD:
                i1 = (gpRegs.top());
                gpRegs.pop();
                i2 = (gpRegs.top());
                gpRegs.pop();
                i3 = cc.newI64();
                cc.xor_(i3, i3);
                cc.idiv(i3, i1, i2);
                gpRegs.push(i3);
                break;
            case BC_DNEG:
                d1 = xmmRegs.top();
                d2 = cc.newXmm();
                cc.movsd(d2, cc.newDoubleConst(kConstScopeLocal, -1.0));
                cc.mulsd(d1, d2);
                break;
            case BC_INEG:
                i1 = gpRegs.top();
                cc.neg(i1);
                break;
            case BC_IAOR:
                i1 = (gpRegs.top());
                gpRegs.pop();
                i2 = (gpRegs.top());
                gpRegs.pop();
                cc.or_(i1, i2);
                gpRegs.push(i1);
                break;
            case BC_IAAND:
                i1 = (gpRegs.top());
                gpRegs.pop();
                i2 = (gpRegs.top());
                gpRegs.pop();
                cc.and_(i1, i2);
                gpRegs.push(i1);
                break;
            case BC_IAXOR:
                i1 = (gpRegs.top());
                gpRegs.pop();
                i2 = (gpRegs.top());
                gpRegs.pop();
                cc.xor_(i1, i2);
                gpRegs.push(i1);
                break;
            case BC_IPRINT:
                i1 = (gpRegs.top());
                gpRegs.pop();
                {
                    const uint8_t args[] = { varTypeToJitType(VT_INT) };
                    FuncSignature sign;
                    sign.init(CallConv::kIdHost, varTypeToJitType(VT_VOID), args, 1);
                    CCFuncCall *call = cc.call(imm_ptr(iprint), sign);
                    call->setArg(0, i1);
                }
                break;
            case BC_DPRINT:
                d1 = xmmRegs.top();
                xmmRegs.pop();
                {
                    const uint8_t args[] = { varTypeToJitType(VT_DOUBLE) };
                    FuncSignature sign;
                    sign.init(CallConv::kIdHost, varTypeToJitType(VT_VOID), args, 1);
                    CCFuncCall *call = cc.call(imm_ptr(dprint), sign);
                    call->setArg(0, d1);
                }
                break;
            case BC_SPRINT:
                i1 = (gpRegs.top());
                gpRegs.pop();
                {
                    const uint8_t args[] = { varTypeToJitType(VT_STRING) };
                    FuncSignature sign;
                    sign.init(CallConv::kIdHost, varTypeToJitType(VT_VOID), args, 1);
                    CCFuncCall *call = cc.call(imm_ptr(sprint), sign);
                    call->setArg(0, i1);
                }
                break;
            case BC_I2D:
                i1 = gpRegs.top();
                gpRegs.pop();
                d1 = cc.newXmm();
                cc.pxor(d1, d1);
                cc.cvtsi2sd(d1, i1);
                xmmRegs.push(d1);
                break;
            case BC_D2I:
                d1 = xmmRegs.top();
                xmmRegs.pop();
                i1 = cc.newI64();
                cc.cvttsd2si(i1, d1);
                gpRegs.push(i1);
                break;
            case BC_S2I:
                // do nothing
                break;
            case BC_SWAP:
                i1 = gpRegs.top();
                gpRegs.pop();
                i2 = gpRegs.top();
                gpRegs.pop();
                gpRegs.push(i1);
                gpRegs.push(i2);
                break;
            case BC_POP:
                if (!xmmRegs.empty()) {
                    xmmRegs.pop();
                } else if (!gpRegs.empty()) {
                    gpRegs.pop();
                }
                break;
            case BC_LOADDVAR0:
                d1 = cc.newXmm();
                cc.movsd(d1, dvars[0]);
                xmmRegs.push(d1);
                break;
            case BC_LOADDVAR1:
                d1 = cc.newXmm();
                cc.movsd(d1, dvars[1]);
                xmmRegs.push(d1);
                break;
            case BC_LOADDVAR2:
                d1 = cc.newXmm();
                cc.movsd(d1, dvars[2]);
                xmmRegs.push(d1);
                break;
            case BC_LOADDVAR3:
                d1 = cc.newXmm();
                cc.movsd(d1, dvars[3]);
                xmmRegs.push(d1);
                break;
            case BC_LOADIVAR0:
            case BC_LOADSVAR0:
                i1 = cc.newI64();
                cc.mov(i1, vars[0]);
                gpRegs.push(i1);
                break;
            case BC_LOADIVAR1:
            case BC_LOADSVAR1:
                i1 = cc.newI64();
                cc.mov(i1, vars[1]);
                gpRegs.push(i1);
                break;
            case BC_LOADIVAR2:
            case BC_LOADSVAR2:
                i1 = cc.newI64();
                cc.mov(i1, vars[2]);
                gpRegs.push(i1);
                break;
            case BC_LOADIVAR3:
            case BC_LOADSVAR3:
                i1 = cc.newI64();
                cc.mov(i1, vars[3]);
                gpRegs.push(i1);
                break;
            case BC_STOREDVAR0:
                d1 = xmmRegs.top();
                xmmRegs.pop();
                cc.movsd(dvars[0], d1);
                break;
            case BC_STOREDVAR1:
                d1 = xmmRegs.top();
                xmmRegs.pop();
                cc.movsd(dvars[1], d1);
                break;
            case BC_STOREDVAR2:
                d1 = xmmRegs.top();
                xmmRegs.pop();
                cc.movsd(dvars[2], d1);
                break;
            case BC_STOREDVAR3:
                d1 = xmmRegs.top();
                xmmRegs.pop();
                cc.movsd(dvars[3], d1);
                break;
            case BC_STOREIVAR0:
            case BC_STORESVAR0:
                i1 = gpRegs.top();
                gpRegs.pop();
                cc.mov(vars[0], i1);
                break;
            case BC_STOREIVAR1:
            case BC_STORESVAR1:
                i1 = gpRegs.top();
                gpRegs.pop();
                cc.mov(vars[1], i1);
                break;
            case BC_STOREIVAR2:
            case BC_STORESVAR2:
                i1 = gpRegs.top();
                gpRegs.pop();
                cc.mov(vars[2], i1);
                break;
            case BC_STOREIVAR3:
            case BC_STORESVAR3:
                i1 = gpRegs.top();
                gpRegs.pop();
                cc.mov(vars[3], i1);
                break;
            case BC_LOADDVAR:
                d1 = cc.newXmm();
                cc.movsd(d1, dvars[bytecode->getInt16(idx+1)]);
                xmmRegs.push(d1);
                break;
            case BC_LOADIVAR:
            case BC_LOADSVAR:
                i1 = cc.newI64();
                cc.mov(i1, vars[bytecode->getInt16(idx+1)]);
                gpRegs.push(i1);
                break;
            case BC_STOREDVAR:
                d1 = xmmRegs.top();
                xmmRegs.pop();
                cc.movsd(dvars[bytecode->getInt16(idx+1)], d1);
                break;
            case BC_STOREIVAR:
            case BC_STORESVAR:
                i1 = gpRegs.top();
                gpRegs.pop();
                cc.mov(vars[bytecode->getInt16(idx+1)], i1);
                break;
                /*
            case BC_LOADCTXDVAR:
                gpRegs.emplace(contexts[bytecode->getInt16(idx+1)].top().
                    getDoubleValue(bytecode->getInt16(idx+3)));
                break;
            case BC_LOADCTXIVAR:
                gpRegs.emplace(contexts[bytecode->getInt16(idx+1)].top().
                    getIntValue(bytecode->getInt16(idx+3)));
                break;
            case BC_LOADCTXSVAR:
                gpRegs.emplace(contexts[bytecode->getInt16(idx+1)].top().
                    getStringValue(bytecode->getInt16(idx+3)));
                break;
            case BC_STORECTXDVAR:
                d1 = gpRegs.top().getDoubleValue();
                gpRegs.pop();
                contexts[bytecode->getInt16(idx+1)].top()
                    .set(bytecode->getInt16(idx+3), d1);
                break;
            case BC_STORECTXIVAR:
                i1 = gpRegs.top().getIntValue();
                gpRegs.pop();
                contexts[bytecode->getInt16(idx+1)].top()
                    .set(bytecode->getInt16(idx+3), i1);
                break;
            case BC_STORECTXSVAR:
                c = gpRegs.top().getStringValue();
                gpRegs.pop();
                contexts[bytecode->getInt16(idx+1)].top()
                    .set(bytecode->getInt16(idx+3), c);
                break;
                */
            case BC_DCMP:
                d1 = xmmRegs.top();
                xmmRegs.pop();
                d2 = xmmRegs.top();
                xmmRegs.pop();
                i1 = cc.newI64();
                {
                    CCFuncCall *call = cc.call(imm_ptr(dcmp), dcmpSign);
                    call->setArg(0, d1);
                    call->setArg(1, d2);
                    call->setRet(0, i1);
                }
                gpRegs.push(i1);
                break;
            case BC_ICMP:
                assert(0);
                break;
            case BC_JA:
                cc.jmp(jumpFrom[idx]);
                break;
#define COMPARE_INTS \
                i1 = gpRegs.top(); \
                gpRegs.pop(); \
                i2 = gpRegs.top(); \
                gpRegs.pop(); \
                cc.cmp(i1, i2); \

            case BC_IFICMPNE:
            case BC_IFICMPE:
            case BC_IFICMPG:
            case BC_IFICMPGE:
            case BC_IFICMPL:
            case BC_IFICMPLE:
                COMPARE_INTS;
                switch (instr) {
                    case BC_IFICMPNE:
                        cc.jne(jumpFrom[idx]);
                        break;
                    case BC_IFICMPE:
                        cc.je(jumpFrom[idx]);
                        break;
                    case BC_IFICMPG:
                        cc.jg(jumpFrom[idx]);
                        break;
                    case BC_IFICMPGE:
                        cc.jge(jumpFrom[idx]);
                        break;
                    case BC_IFICMPL:
                        cc.jl(jumpFrom[idx]);
                        break;
                    case BC_IFICMPLE:
                        cc.jle(jumpFrom[idx]);
                        break;
                    default:
                        assert(0);
                        break;
                }

                break;
#undef COMPARE_INTS
/*
            case BC_DUMP:
                cout << gpRegs.top();
                break;
*/
            case BC_STOP:
                cc.ret();
                break;
            case BC_CALL: {
                uint16_t id = bytecode->getInt16(idx + 1);
                BytecodeFunction *fn = (BytecodeFunction*)
                    (bytecodeCode->functionById(id));
                X86Gp ftable = cc.newUIntPtr();
                cc.mov(ftable, imm_ptr(funcTable));
                CCFuncCall *call = cc.call(
                    x86::ptr(ftable, int(id * sizeof(void*))),
                    sigs[id]);
                for (uint16_t i = 0; i < fn->parametersNumber(); ++i) {
                    uint16_t ix = fn->parametersNumber()-i-1;
                    switch (fn->parameterType(ix)) {
                        case VT_DOUBLE: {
                            d1 = xmmRegs.top();
                            xmmRegs.pop();
                            call->setArg(ix, d1);
                            break;
                        }
                        case VT_INT:
                        case VT_STRING: {
                            i1 = gpRegs.top();
                            gpRegs.pop();
                            call->setArg(ix, i1);
                            break;
                        }
                        case VT_INVALID:
                        case VT_VOID:
                            assert(0);
                            break;
                    }
                }
                switch (fn->returnType()) {
                    case VT_DOUBLE:
                        d1 = cc.newXmm();
                        call->setRet(0, d1);
                        xmmRegs.push(d1);
                        break;
                    case VT_INT:
                    case VT_STRING:
                        i1 = cc.newI64();
                        call->setRet(0, i1);
                        gpRegs.push(i1);
                        break;
                    case VT_VOID:
                        break;
                    case VT_INVALID:
                        assert(0);
                        break;
                }
                break;
            }
/*
            case BC_CALLNATIVE:
                break;
*/
            case BC_RETURN:
                switch (function.returnType()) {
                    case VT_DOUBLE:
                        d1 = xmmRegs.top();
                        cc.ret(d1);
                        break;
                    case VT_INT:
                    case VT_STRING:
                        i1 = gpRegs.top();
                        cc.ret(i1);
                        break;
                    case VT_VOID:
                        cc.ret();
                        break;
                    case VT_INVALID:
                        assert(0);
                        break;
                }
                cc.jmp(funcEnd);
                break;
            case BC_BREAK:
                cc.int_(3);
                break;
            default:
                cerr << insn_names(instr) << endl;
                assert(0);
                break;
        }
    }
    cc.endFunc();
    cc.finalize();

    void *myFn;
    Error err = rt.add(&myFn, &code);
    if (err) {
        printf("ASMJIT ERROR: 0x%08X [%s]\n", err,
            DebugUtils::errorAsString(err));
        assert(0);
    }

    return myFn;
}

JitEnvironment::JitEnvironment(InterpreterCodeImpl& code) {
    inner = new JitEnvImpl(code);
}

JitEnvironment::~JitEnvironment() {
    delete inner;
}

}
