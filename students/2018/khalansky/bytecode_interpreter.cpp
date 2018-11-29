#include "bytecode_interpreter.hpp"
#include <stack>
#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbool-operation"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#include <asmjit/asmjit.h>
#pragma GCC diagnostic pop

using namespace std;

namespace mathvm {

uint8_t insn_sizes(Instruction instr) {
    switch (instr) {
#define ENUM_ELEM(b, d, i) case BC_##b: return i;
    FOR_BYTECODES(ENUM_ELEM)
#undef ENUM_ELEM
        default:
            assert(0);
    }
}

const char *insn_names(Instruction instr) {
    switch (instr) {
#define ENUM_ELEM(b, d, i) case BC_##b: return #b;
    FOR_BYTECODES(ENUM_ELEM)
#undef ENUM_ELEM
        default:
            assert(0);
    }
}

class Value {
    VarType _type;
    union {
        double _doubleValue;
        int64_t _intValue;
        const char* _stringValue;
    };

public:
    Value(const char* stringValue):
      _type(VT_STRING), _stringValue(stringValue) { }
    Value(double doubleValue):
      _type(VT_DOUBLE), _doubleValue(doubleValue) { }
    Value(int64_t intValue):
      _type(VT_INT), _intValue(intValue) { }

    double getDoubleValue() const {
        assert(_type == VT_DOUBLE);
        return _doubleValue;
    }

    int64_t getIntValue() const {
        assert(_type == VT_INT);
        return _intValue;
    }

    const char* getStringValue() const {
        assert(_type == VT_STRING);
        return _stringValue;
    }

    friend ostream& operator<<(ostream &os, const Value& value) {
        switch (value._type) {
            case VT_STRING:
                os << value._stringValue;
                break;
            case VT_INT:
                os << value._intValue;
                break;
            case VT_DOUBLE:
                os << value._doubleValue;
                break;
            default:
                assert(0);
        }
        return os;
    }
};

static void callNative(const Code *code, uint16_t id, stack<Value>& stk) {

    using namespace asmjit;

    const string *name;
    const Signature *signature;
    const void* fn = code->nativeById(id, &signature, &name);
    assert(fn);

    JitRuntime rt;

    CodeHolder holder;
    holder.init(rt.getCodeInfo());

    uint8_t argtp[signature->size()];
    for (size_t i = 0; i < signature->size(); ++i) {
        switch ((*signature)[i].first) {
            case VT_DOUBLE:
                argtp[i] = TypeIdOf<double>::kTypeId;
                break;
            case VT_INT:
                argtp[i] = TypeIdOf<int64_t>::kTypeId;
                break;
            case VT_STRING:
                argtp[i] = TypeIdOf<const char*>::kTypeId;
                break;
            case VT_VOID:
                argtp[i] = TypeIdOf<void>::kTypeId;
                break;
            default:
                assert(0);
                break;
        }
    }

    X86Compiler a(&holder);
    FuncSignature thisFn;
    thisFn.init(CallConv::kIdHost, argtp[0], nullptr, 0);
    a.addFunc(thisFn);
    FuncSignature asmjitSign;
    asmjitSign.init(CallConv::kIdHost, argtp[0], argtp + 1, signature->size()-1);
    stack<X86Reg> args;
    for (size_t i = signature->size()-1; i > 0; --i) {
        switch ((*signature)[i].first) {
            case VT_DOUBLE: {
                X86Xmm val = a.newXmm();
                X86Mem c0(a.newDoubleConst(kConstScopeLocal,
                    stk.top().getDoubleValue()));
                a.movsd(val, c0);
                args.push(val);
                break;
            }
            case VT_INT: {
                X86Gp val = a.newI64();
                a.mov(val, stk.top().getIntValue());
                args.push(val);
                break;
            }
            case VT_STRING: {
                X86Gp val = a.newUIntPtr();
                a.mov(val, (uint64_t)stk.top().getStringValue());
                args.push(val);
                break;
            }
            default:
                assert(0);
                break;
        }
        stk.pop();
    }
    CCFuncCall *call = a.call(imm_ptr(fn), asmjitSign);
    for (size_t i = 0; i < signature->size()-1; ++i) {
        call->setArg(i, args.top());
        args.pop();
    }
    switch ((*signature)[0].first) {
        case VT_DOUBLE: {
            X86Xmm retVal = a.newXmm();
            call->setRet(0, retVal);
            a.ret(retVal);
            break;
        }
        case VT_INT:
        case VT_STRING: {
            X86Gp retVal = a.newI64();
            call->setRet(0, retVal);
            a.ret(retVal);
            break;
        }
        case VT_VOID:
            a.ret();
            break;
        default:
            assert(0);
    }
    a.endFunc();
    a.finalize();

    void *myFn;
    Error err = rt.add(&myFn, &holder);
    if (err) {
        printf("ASMJIT ERROR: 0x%08X [%s]\n", err, DebugUtils::errorAsString(err));
        assert(0);
    }
    switch ((*signature)[0].first) {
        case VT_DOUBLE:
            stk.emplace(((double (*)())myFn)());
            break;
        case VT_INT:
            stk.emplace(((int64_t (*)())myFn)());
            break;
        case VT_STRING:
            stk.emplace(((const char *(*)())myFn)());
            break;
        case VT_VOID:
            ((void *(*)())myFn)();
            break;
        default:
            assert(0);
            break;
    }
    rt.release(myFn);
}

class Context {
private:
    vector<Var*> values;
public:

    Context(uint32_t localno) : values(localno) { }

    ~Context() {
        for (auto c : values) {
            if (c) {
                delete c;
            }
        }
    }

    double getDoubleValue(uint16_t id) const {
        // huh?
        if (!values[id]) {
            return 0.0;
        }
        return values[id]->getDoubleValue();
    }

    int64_t getIntValue(uint16_t id) const {
        if (!values[id]) {
            return 0;
        }
        return values[id]->getIntValue();
    }

    const char* getStringValue(uint16_t id) const {
        if (!values[id]) {
            return nullptr;
        }
        return values[id]->getStringValue();
    }

    void set(uint16_t id, double value) {
        if (!values[id]) {
            values[id] = new Var(VT_DOUBLE, "");
        }
        values[id]->setDoubleValue(value);
    }

    void set(uint16_t id, int64_t value) {
        if (!values[id]) {
            values[id] = new Var(VT_INT, "");
        }
        values[id]->setIntValue(value);
    }

    void set(uint16_t id, const char *value) {
        if (!values[id]) {
            values[id] = new Var(VT_STRING, "");
        }
        values[id]->setStringValue(value);
    }
};

Status* MathvmCode::execute(vector<Var*>& vars) {
    const TranslatedFunction *top = functionByName("<top>");

    int max_fn_id = 0;
    {
        Code::FunctionIterator fnit(this);
        while (fnit.hasNext()) {
            TranslatedFunction *fn = fnit.next();
            max_fn_id = max(max_fn_id, fn->id() + 1);
        }
    }

    vector<stack<Context>> contexts(max_fn_id);
    stack<pair<BytecodeFunction *, uint32_t> > callStack;
    stack<Value> stack;

    BytecodeFunction *curFn = (BytecodeFunction*)top;

    Bytecode *code = curFn->bytecode();
    contexts[curFn->id()].emplace(curFn->localsNumber());
    Context *context = &contexts[curFn->id()].top();
    uint32_t idx = 0;

    Value v1((int64_t)0LL), v2((int64_t)0LL);
    double d1, d2;
    int64_t i1, i2;
    const char *c;
#ifdef DEBUG
/*
    for (size_t j = 0; j < code->length(); ++j) {
        cerr << j << " ";
        Instruction instr = (Instruction)code->get(j);
        cerr << insn_names(instr);
        for (size_t i = 1; i < insn_sizes(instr); ++i) {
            cerr << " " << (int)(code->get(j+i));
        }
        j += insn_sizes(instr)-1;
        cerr << endl;
    }
    cerr << "END OF PROGRAM" << endl;
*/
#endif
    while (true) {
        Instruction instr = (Instruction)code->get(idx);
#ifdef DEBUG
        cerr << "#" << stack.size() << " " << functionById(curFn->id())->name() << "[" << callStack.size() << "]; next :";
        cerr << idx << " " << insn_names(instr);
        for (size_t i = 1; i < insn_sizes(instr); ++i) {
            cerr << " " << (int)(code->get(idx+i));
        }
        cerr << endl;
#endif

        switch (instr) {
            case BC_INVALID:
                return Status::Error("Invalid bytecode found");
                break;
            case BC_DLOAD:
                stack.emplace(code->getDouble(idx+1));
                idx += insn_sizes(instr);
                break;
            case BC_ILOAD:
                stack.emplace(code->getInt64(idx+1));
                idx += insn_sizes(instr);
                break;
            case BC_SLOAD:
                stack.emplace(constantById(code->getInt16(idx+1)).c_str());
                idx += insn_sizes(instr);
                break;
            case BC_DLOAD0:
                stack.emplace(0.0);
                idx += insn_sizes(instr);
                break;
            case BC_ILOAD0:
                stack.emplace((int64_t)0LL);
                idx += insn_sizes(instr);
                break;
            case BC_SLOAD0:
                stack.emplace("");
                idx += insn_sizes(instr);
                break;
            case BC_DLOAD1:
                stack.emplace(1.0);
                idx += insn_sizes(instr);
                break;
            case BC_ILOAD1:
                stack.emplace((int64_t)1LL);
                idx += insn_sizes(instr);
                break;
            case BC_DLOADM1:
                stack.emplace(-1.0);
                idx += insn_sizes(instr);
                break;
            case BC_ILOADM1:
                stack.emplace((int64_t)-1LL);
                idx += insn_sizes(instr);
                break;
            case BC_DADD:
                d1 = stack.top().getDoubleValue();
                stack.pop();
                d2 = stack.top().getDoubleValue();
                stack.pop();
                stack.emplace(d1 + d2);
                idx += insn_sizes(instr);
                break;
            case BC_IADD:
                i1 = stack.top().getIntValue();
                stack.pop();
                i2 = stack.top().getIntValue();
                stack.pop();
                stack.emplace(i1 + i2);
                idx += insn_sizes(instr);
                break;
            case BC_DSUB:
                d1 = stack.top().getDoubleValue();
                stack.pop();
                d2 = stack.top().getDoubleValue();
                stack.pop();
                stack.emplace(d1 - d2);
                idx += insn_sizes(instr);
                break;
            case BC_ISUB:
                i1 = stack.top().getIntValue();
                stack.pop();
                i2 = stack.top().getIntValue();
                stack.pop();
                stack.emplace(i1 - i2);
                idx += insn_sizes(instr);
                break;
            case BC_DMUL:
                d1 = stack.top().getDoubleValue();
                stack.pop();
                d2 = stack.top().getDoubleValue();
                stack.pop();
                stack.emplace(d1 * d2);
                idx += insn_sizes(instr);
                break;
            case BC_IMUL:
                i1 = stack.top().getIntValue();
                stack.pop();
                i2 = stack.top().getIntValue();
                stack.pop();
                stack.emplace(i1 * i2);
                idx += insn_sizes(instr);
                break;
            case BC_DDIV:
                d1 = stack.top().getDoubleValue();
                stack.pop();
                d2 = stack.top().getDoubleValue();
                stack.pop();
                stack.emplace(d1 / d2);
                idx += insn_sizes(instr);
                break;
            case BC_IDIV:
                i1 = stack.top().getIntValue();
                stack.pop();
                i2 = stack.top().getIntValue();
                stack.pop();
                stack.emplace(i1 / i2);
                idx += insn_sizes(instr);
                break;
            case BC_IMOD:
                i1 = stack.top().getIntValue();
                stack.pop();
                i2 = stack.top().getIntValue();
                stack.pop();
                stack.emplace(i1 % i2);
                idx += insn_sizes(instr);
                break;
            case BC_DNEG:
                d1 = stack.top().getDoubleValue();
                stack.pop();
                stack.emplace(-d1);
                idx += insn_sizes(instr);
                break;
            case BC_INEG:
                i1 = stack.top().getIntValue();
                stack.pop();
                stack.emplace(-i1);
                idx += insn_sizes(instr);
                break;
            case BC_IAOR:
                i1 = stack.top().getIntValue();
                stack.pop();
                i2 = stack.top().getIntValue();
                stack.pop();
                stack.emplace(i1 | i2);
                idx += insn_sizes(instr);
                break;
            case BC_IAAND:
                i1 = stack.top().getIntValue();
                stack.pop();
                i2 = stack.top().getIntValue();
                stack.pop();
                stack.emplace(i1 & i2);
                idx += insn_sizes(instr);
                break;
            case BC_IAXOR:
                i1 = stack.top().getIntValue();
                stack.pop();
                i2 = stack.top().getIntValue();
                stack.pop();
                stack.emplace(i1 ^ i2);
                idx += insn_sizes(instr);
                break;
            case BC_IPRINT:
                i1 = stack.top().getIntValue();
                stack.pop();
                cout << i1;
                idx += insn_sizes(instr);
                break;
            case BC_DPRINT:
                d1 = stack.top().getDoubleValue();
                stack.pop();
                cout << d1;
                idx += insn_sizes(instr);
                break;
            case BC_SPRINT:
                c = stack.top().getStringValue();
                stack.pop();
                cout << c;
                idx += insn_sizes(instr);
                break;
            case BC_I2D:
                i1 = stack.top().getIntValue();
                stack.pop();
                stack.emplace((double)i1);
                idx += insn_sizes(instr);
                break;
            case BC_D2I:
                d1 = stack.top().getDoubleValue();
                stack.pop();
                stack.emplace((int64_t)d1);
                idx += insn_sizes(instr);
                break;
            case BC_S2I:
                c = stack.top().getStringValue();
                stack.pop();
                stack.emplace((int64_t)c);
                idx += insn_sizes(instr);
                break;
            case BC_SWAP:
                v1 = stack.top();
                stack.pop();
                v2 = stack.top();
                stack.pop();
                stack.push(v1);
                stack.push(v2);
                idx += insn_sizes(instr);
                break;
            case BC_POP:
                stack.pop();
                idx += insn_sizes(instr);
                break;
            case BC_LOADDVAR0:
                stack.emplace(context->getDoubleValue(0));
                idx += insn_sizes(instr);
                break;
            case BC_LOADDVAR1:
                stack.emplace(context->getDoubleValue(1));
                idx += insn_sizes(instr);
                break;
            case BC_LOADDVAR2:
                stack.emplace(context->getDoubleValue(2));
                idx += insn_sizes(instr);
                break;
            case BC_LOADDVAR3:
                stack.emplace(context->getDoubleValue(3));
                idx += insn_sizes(instr);
                break;
            case BC_LOADIVAR0:
                stack.emplace(context->getIntValue(0));
                idx += insn_sizes(instr);
                break;
            case BC_LOADIVAR1:
                stack.emplace(context->getIntValue(1));
                idx += insn_sizes(instr);
                break;
            case BC_LOADIVAR2:
                stack.emplace(context->getIntValue(2));
                idx += insn_sizes(instr);
                break;
            case BC_LOADIVAR3:
                stack.emplace(context->getIntValue(3));
                idx += insn_sizes(instr);
                break;
            case BC_LOADSVAR0:
                stack.emplace(context->getStringValue(0));
                idx += insn_sizes(instr);
                break;
            case BC_LOADSVAR1:
                stack.emplace(context->getStringValue(1));
                idx += insn_sizes(instr);
                break;
            case BC_LOADSVAR2:
                stack.emplace(context->getStringValue(2));
                idx += insn_sizes(instr);
                break;
            case BC_LOADSVAR3:
                stack.emplace(context->getStringValue(3));
                idx += insn_sizes(instr);
                break;
            case BC_STOREDVAR0:
                d1 = stack.top().getDoubleValue();
                stack.pop();
                context->set(0, d1);
                idx += insn_sizes(instr);
                break;
            case BC_STOREDVAR1:
                d1 = stack.top().getDoubleValue();
                stack.pop();
                context->set(1, d1);
                idx += insn_sizes(instr);
                break;
            case BC_STOREDVAR2:
                d1 = stack.top().getDoubleValue();
                stack.pop();
                context->set(2, d1);
                idx += insn_sizes(instr);
                break;
            case BC_STOREDVAR3:
                d1 = stack.top().getDoubleValue();
                stack.pop();
                context->set(3, d1);
                idx += insn_sizes(instr);
                break;
            case BC_STOREIVAR0:
                i1 = stack.top().getIntValue();
                stack.pop();
                context->set(0, i1);
                idx += insn_sizes(instr);
                break;
            case BC_STOREIVAR1:
                i1 = stack.top().getIntValue();
                stack.pop();
                context->set(1, i1);
                idx += insn_sizes(instr);
                break;
            case BC_STOREIVAR2:
                i1 = stack.top().getIntValue();
                stack.pop();
                context->set(2, i1);
                idx += insn_sizes(instr);
                break;
            case BC_STOREIVAR3:
                i1 = stack.top().getIntValue();
                stack.pop();
                context->set(3, i1);
                idx += insn_sizes(instr);
                break;
            case BC_STORESVAR0:
                c = stack.top().getStringValue();
                stack.pop();
                context->set(0, c);
                idx += insn_sizes(instr);
                break;
            case BC_STORESVAR1:
                c = stack.top().getStringValue();
                stack.pop();
                context->set(1, c);
                idx += insn_sizes(instr);
                break;
            case BC_STORESVAR2:
                c = stack.top().getStringValue();
                stack.pop();
                context->set(2, c);
                idx += insn_sizes(instr);
                break;
            case BC_STORESVAR3:
                c = stack.top().getStringValue();
                stack.pop();
                context->set(3, c);
                idx += insn_sizes(instr);
                break;
            case BC_LOADDVAR:
                stack.emplace(context->getDoubleValue(code->getInt16(idx+1)));
                idx += insn_sizes(instr);
                break;
            case BC_LOADIVAR:
                stack.emplace(context->getIntValue(code->getInt16(idx+1)));
                idx += insn_sizes(instr);
                break;
            case BC_LOADSVAR:
                stack.emplace(context->getStringValue(code->getInt16(idx+1)));
                idx += insn_sizes(instr);
                break;
            case BC_STOREDVAR:
                d1 = stack.top().getDoubleValue();
                stack.pop();
                context->set(code->getInt16(idx+1), d1);
                idx += insn_sizes(instr);
                break;
            case BC_STOREIVAR:
                i1 = stack.top().getIntValue();
                stack.pop();
                context->set(code->getInt16(idx+1), i1);
                idx += insn_sizes(instr);
                break;
            case BC_STORESVAR:
                c = stack.top().getStringValue();
                stack.pop();
                context->set(code->getInt16(idx+1), c);
                idx += insn_sizes(instr);
                break;
            case BC_LOADCTXDVAR:
                stack.emplace(contexts[code->getInt16(idx+1)].top().
                    getDoubleValue(code->getInt16(idx+3)));
                idx += insn_sizes(instr);
                break;
            case BC_LOADCTXIVAR:
                stack.emplace(contexts[code->getInt16(idx+1)].top().
                    getIntValue(code->getInt16(idx+3)));
                idx += insn_sizes(instr);
                break;
            case BC_LOADCTXSVAR:
                stack.emplace(contexts[code->getInt16(idx+1)].top().
                    getStringValue(code->getInt16(idx+3)));
                idx += insn_sizes(instr);
                break;
            case BC_STORECTXDVAR:
                d1 = stack.top().getDoubleValue();
                stack.pop();
                contexts[code->getInt16(idx+1)].top()
                    .set(code->getInt16(idx+3), d1);
                idx += insn_sizes(instr);
                break;
            case BC_STORECTXIVAR:
                i1 = stack.top().getIntValue();
                stack.pop();
                contexts[code->getInt16(idx+1)].top()
                    .set(code->getInt16(idx+3), i1);
                idx += insn_sizes(instr);
                break;
            case BC_STORECTXSVAR:
                c = stack.top().getStringValue();
                stack.pop();
                contexts[code->getInt16(idx+1)].top()
                    .set(code->getInt16(idx+3), c);
                idx += insn_sizes(instr);
                break;
            case BC_DCMP:
                d1 = stack.top().getDoubleValue();
                stack.pop();
                d2 = stack.top().getDoubleValue();
                stack.pop();
                if (d1 < d2) {
                    stack.emplace((int64_t)-1LL);
                } else if (d1 == d2) {
                    stack.emplace((int64_t)0LL);
                } else {
                    stack.emplace((int64_t)1LL);
                }
                idx += insn_sizes(instr);
                break;
            case BC_ICMP:
                i1 = stack.top().getIntValue();
                stack.pop();
                i2 = stack.top().getIntValue();
                stack.pop();
                if (i1 < i2) {
                    stack.emplace((int64_t)-1LL);
                } else if (i1 == i2) {
                    stack.emplace((int64_t)0LL);
                } else {
                    stack.emplace((int64_t)1LL);
                }
                idx += insn_sizes(instr);
                break;
            case BC_JA:
                idx += 1 + code->getInt16(idx+1);
                break;
#define COMPARE_INTS(cond) \
                i1 = stack.top().getIntValue(); \
                stack.pop(); \
                i2 = stack.top().getIntValue(); \
                stack.pop(); \
                if (cond) { \
                    idx += 1 + code->getInt16(idx+1); \
                } else { \
                    idx += insn_sizes(instr); \
                }
            case BC_IFICMPNE:
                COMPARE_INTS(i1 != i2)
                break;
            case BC_IFICMPE:
                COMPARE_INTS(i1 == i2)
                break;
            case BC_IFICMPG:
                COMPARE_INTS(i1 > i2)
                break;
            case BC_IFICMPGE:
                COMPARE_INTS(i1 >= i2)
                break;
            case BC_IFICMPL:
                COMPARE_INTS(i1 < i2)
                break;
            case BC_IFICMPLE:
                COMPARE_INTS(i1 <= i2)
                break;
#undef COMPARE_INTS
            case BC_DUMP:
                cout << stack.top();
                idx += insn_sizes(instr);
                break;
            case BC_STOP:
                return Status::Ok();
                break;
            case BC_CALL:
                callStack.emplace(curFn, idx + insn_sizes(instr));
                curFn = (BytecodeFunction*)functionById(code->getInt16(idx + 1));
                code = curFn->bytecode();
                contexts[curFn->id()].emplace(curFn->localsNumber());
                context = &contexts[curFn->id()].top();
                idx = 0;
                break;
            case BC_CALLNATIVE:
                callNative(this, code->getInt16(idx + 1), stack);
            case BC_RETURN:
                contexts[curFn->id()].pop();
                curFn = callStack.top().first;
                idx = callStack.top().second;
                callStack.pop();
                code = curFn->bytecode();
                context = &contexts[curFn->id()].top();
                break;
            case BC_BREAK:
                asm("int $3");
                break;
            default:
                assert(0);
                break;
        }
    }
}

}
