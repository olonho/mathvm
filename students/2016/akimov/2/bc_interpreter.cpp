#include "bc_interpreter.h"
#include <ast.h>

#include <stack>
#include <cinttypes>

using namespace mathvm;
using namespace asmjit;

struct InterpreterError : public ErrorInfoHolder {};

static void error(uint16_t func = 0, const string& message = "") {
    (new InterpreterError())->error(0, "InterpreterError at function %d: %s", func, message.c_str());
}

union val {
    val() : ival(0) {}
    val(int64_t val) : ival(val) {}
    val(double val) : dval(val) {}
    val(char const* val) : sval(val) {}

    operator int64_t() { return ival; }
    operator double() { return dval; }
    operator char const*() { return sval; }

    int64_t ival;
    double dval;
    char const* sval;
};

struct CTX {
    CTX(InterpreterCodeImpl* code)
        : _code(code)
        , _callStackLevel(0)
    {
        for (Code::FunctionIterator it(code); it.hasNext();) {
            TranslatedFunction* function = it.next();
            uint16_t scope = function->scopeId();
            if (scope >= _vars.size()) {
                _vars.resize(scope + 1);
            }
            if (scope >= _scopeSize.size()) {
                _scopeSize.resize(scope + 1);
            }
            _scopeSize[scope] = function->localsNumber();
        }
    }
private:
    InterpreterCodeImpl* _code;
    uint32_t _callStackLevel;

    vector<vector<val>> _vars;
    vector<uint16_t> _scopeSize;
    stack<val> _stack;
    int64_t _iregs[4];
    double _dregs[4];
    const char* _sregs[4];
public:

    void pushScope(uint16_t scope) {
        _vars[scope].resize(_vars[scope].size() + _scopeSize[scope]);
    }

    void popScope(uint16_t scope) {
        _vars[scope].resize(_vars[scope].size() - _scopeSize[scope]);
    }

    val popVal() {
        val value = _stack.top(); _stack.pop();
        return value;
    }
    void pushVal(val value) {
        _stack.push(value);
    }
    val getVal(uint16_t scope, uint16_t id) {
        return _vars[scope][_vars[scope].size() - 1 - id];
    }
    void setVal(uint16_t scope, uint16_t id, val v) {
        _vars[scope][_vars[scope].size() - 1 - id] = v;
    }

    void call(uint16_t functionId) {
        if (_callStackLevel++ > 10000) error(functionId, "stack overflow");

        BytecodeFunction* func = _code->functionById(functionId);
        Bytecode* bc = func->bytecode();

        uint16_t args = func->parametersNumber();
        uint16_t scopeId = func->scopeId();

        pushScope(scopeId);

        for (uint16_t i = 0; i < args; ++i) {
            setVal(scopeId, args - i - 1, popVal());
        }

        exec(bc, scopeId);

        popScope(scopeId);

        --_callStackLevel;
    }

    void exec(Bytecode* bc, uint16_t scopeId) {
        uint32_t i = 0;
        while (i < bc->length()) {
            Instruction insn = bc->getInsn(i); ++i;

            switch (insn) {
                case BC_DLOAD:
                    pushVal(bc->getDouble(i));
                    i += sizeof(double);
                    break;
                case BC_ILOAD:
                    pushVal(bc->getInt64(i));
                    i += sizeof(int64_t);
                    break;
                case BC_SLOAD:
                    pushVal(_code->constantById(bc->getUInt16(i)).c_str());
                    i += sizeof(uint16_t);
                    break;

#define OPL(v) { pushVal(v); break; }
#define OPS(v) { v = popVal(); break; }
#define OP2(T, o) { T a = popVal(); T b = popVal(); pushVal(a o b); break; }
#define OP1(T, o) { T a = popVal(); pushVal(o (a)); break; }
//#define OPP(T, t) { T a = popVal(); fprintf(stdout, "%" t, a); break; }
//#define OPP(T, t) { T a = popVal(); cout << a; fflush(stdout); break; }
#define OPP(T, t) { T a = popVal(); cout << a; break; }

                case BC_DLOAD0: OPL(0.0);
                case BC_ILOAD0: OPL(int64_t(0));
                case BC_SLOAD0: OPL(_code->constantById(0).c_str());
                case BC_DLOAD1: OPL(1.0);
                case BC_ILOAD1: OPL(int64_t(1));
                case BC_DLOADM1: OPL(-1.0);
                case BC_ILOADM1: OPL(int64_t(-1));

                case BC_DADD: OP2(double, +);
                case BC_IADD: OP2(int64_t, +);
                case BC_DSUB: OP2(double, -);
                case BC_ISUB: OP2(int64_t, -);
                case BC_DMUL: OP2(double, *);
                case BC_IMUL: OP2(int64_t, *);
                case BC_DDIV: OP2(double, /);
                case BC_IDIV: OP2(int64_t, /);
                case BC_IMOD: OP2(int64_t, %);
                case BC_DNEG: OP1(double, -);
                case BC_INEG: OP1(int64_t, -);
                case BC_IAOR: OP2(int64_t, |);
                case BC_IAAND: OP2(int64_t, &);
                case BC_IAXOR: OP2(int64_t, ^);
                case BC_IPRINT: OPP(int64_t, PRId64);
                case BC_DPRINT: OPP(double, "f");
                case BC_SPRINT: OPP(const char*, "s");
                case BC_I2D: OP1(int64_t, static_cast<double>);
                case BC_D2I: OP1(double, static_cast<int64_t>);
                case BC_S2I: OP1(const char*, reinterpret_cast<int64_t>);

                case BC_SWAP: { val a = popVal(); val b = popVal(); pushVal(a); pushVal(b); break; }
                case BC_POP: popVal(); break;

                case BC_LOADDVAR0: OPL(_dregs[0]);
                case BC_LOADDVAR1: OPL(_dregs[1]);
                case BC_LOADDVAR2: OPL(_dregs[2]);
                case BC_LOADDVAR3: OPL(_dregs[3]);
                case BC_LOADIVAR0: OPL(_iregs[0]);
                case BC_LOADIVAR1: OPL(_iregs[1]);
                case BC_LOADIVAR2: OPL(_iregs[2]);
                case BC_LOADIVAR3: OPL(_iregs[3]);
                case BC_LOADSVAR0: OPL(_sregs[0]);
                case BC_LOADSVAR1: OPL(_sregs[1]);
                case BC_LOADSVAR2: OPL(_sregs[2]);
                case BC_LOADSVAR3: OPL(_sregs[3]);
                case BC_STOREDVAR0: OPS(_dregs[0]);
                case BC_STOREDVAR1: OPS(_dregs[1]);
                case BC_STOREDVAR2: OPS(_dregs[2]);
                case BC_STOREDVAR3: OPS(_dregs[3]);
                case BC_STOREIVAR0: OPS(_iregs[0]);
                case BC_STOREIVAR1: OPS(_iregs[1]);
                case BC_STOREIVAR2: OPS(_iregs[2]);
                case BC_STOREIVAR3: OPS(_iregs[3]);
                case BC_STORESVAR0: OPS(_sregs[0]);
                case BC_STORESVAR1: OPS(_sregs[1]);
                case BC_STORESVAR2: OPS(_sregs[2]);
                case BC_STORESVAR3: OPS(_sregs[3]);
#undef OPP
#undef OP1
#undef OP2
#undef OPS
#undef OPL

                case BC_LOADDVAR:
                case BC_LOADIVAR:
                case BC_LOADSVAR:
                    // unused
                    break;

                case BC_LOADCTXDVAR:
                case BC_LOADCTXIVAR:
                case BC_LOADCTXSVAR: {
                    uint16_t scope = bc->getUInt16(i); i += sizeof(uint16_t);
                    uint16_t var = bc->getUInt16(i); i+= sizeof(uint16_t);
                    pushVal(getVal(scope, var));
                    break;
                }
                case BC_STORECTXDVAR:
                case BC_STORECTXIVAR:
                case BC_STORECTXSVAR: {
                    uint16_t scope = bc->getUInt16(i); i += sizeof(uint16_t);
                    uint16_t var = bc->getUInt16(i); i += sizeof(uint16_t);
                    setVal(scope, var, popVal());
                    break;
                }

                case BC_DCMP: cmp<double>(); break;
                case BC_ICMP: cmp<int64_t>(); break;

                case BC_JA: {
                    i += bc->getInt16(i);
                    break;
                }

#define JMP(o) { int64_t u = popVal(); int64_t l = popVal();  \
                 int16_t offset = bc->getInt16(i);            \
                 if (u o l) i += offset;                      \
                 else i += sizeof(int16_t);                   \
                 pushVal(l); pushVal(u);                      \
                 break; }

                case BC_IFICMPNE: JMP(!=);
                case BC_IFICMPE:  JMP(==);
                case BC_IFICMPG:  JMP(>);
                case BC_IFICMPGE: JMP(>=);
                case BC_IFICMPL:  JMP(<);
                case BC_IFICMPLE: JMP(<=);
#undef JMP

                case BC_DUMP: {
                    // unused
                    //val a = popVal();
                    //pushVal(a);
                    //pushVal(a);
                    break;
                }

                case BC_STOP: {
                    assert(_stack.empty());
                    exit(0);
                    break;
                }

                case BC_CALL: {
                    uint16_t functionId = bc->getUInt16(i);
                    i += sizeof(uint16_t);
                    call(functionId);
                    break;
                }

                case BC_CALLNATIVE: {
                    uint16_t functionId = bc->getUInt16(i);
                    i += sizeof(uint16_t);
                    callNative(functionId, scopeId);
                    break;
                }

                case BC_RETURN:
                    return;

                case BC_BREAK:
                    break;

                case BC_INVALID:
                default:
                    error();
            }
        }
    }

private:
    template<class T> void cmp() {
        T a = popVal();
        T b = popVal();
        pushVal(b);
        pushVal(a);
        pushVal(a == b ? 0 : a < b ? int64_t(-1) : int64_t(1));
    }

    void callNative(uint16_t functionId, uint16_t scopeId) {
        const Signature* signature;
        const string* name;
        const void* nativePtr = _code->nativeById(functionId, &signature, &name);
        mathvm::VarType retType = signature->at(0).first;

        string data;
        appendValToMem(data, retType, val());
        for (size_t j = 0; j < signature->size() - 1; ++j) {
            auto var = signature->at(j + 1);
            appendValToMem(data, var.first, getVal(scopeId, j));
        }

        asmjit_cast<void (*)(const char*)>(nativePtr)(data.data());
        if (retType != VT_VOID) {
            pushVal(getValFromMem(data, retType));
        }
    }

    static void appendValToMem(string& data, mathvm::VarType type, val value) {
        size_t size = data.size();
        switch (type) {
            case VT_INT: {
                data.resize(data.size() + sizeof(int64_t));
                int64_t *ptr = (int64_t*)(data.data() + size);
                *ptr = value;
                break;
            }
            case VT_DOUBLE: {
                data.resize(data.size() + sizeof(int64_t));
                double *ptr = (double*)(data.data() + size);
                *ptr = value;
                break;
            }
            case VT_STRING: {
                data.resize(data.size() + sizeof(int64_t));
                const char* *ptr = (const char* *)(data.data() + size);
                *ptr = value;
                break;
            }
            case VT_VOID: {
                break;
            }
        }
    }

    static val getValFromMem(string& data, mathvm::VarType type, size_t offset = 0) {
        switch (type) {
            case VT_INT:
                return *(int64_t *)(data.data() + offset);
            case VT_DOUBLE:
                return *(double *)(data.data() + offset);
            case VT_STRING:
                return *(const char* *)(data.data() + offset);
        }
        return val();
    }
};

Status* InterpreterCodeImpl::execute(vector<Var*>& vars) {
    BytecodeFunction* topFunction = functionByName(AstFunction::top_name);

    try {
        CTX ctx(this);
        //ctx.call(topFunction->id());

        uint16_t scopeId = topFunction->scopeId();
        ctx.pushScope(scopeId);
        for (size_t i = 0; i< vars.size(); ++i) {
            switch (vars[i]->type()) {
                case VT_INT:
                    ctx.setVal(scopeId, i, vars[i]->getIntValue());
                    break;
                case VT_DOUBLE:
                    ctx.setVal(scopeId, i, vars[i]->getDoubleValue());
                    break;
                case VT_STRING:
                    ctx.setVal(scopeId, i, vars[i]->getStringValue());
                    break;
            }
        }
        ctx.exec(topFunction->bytecode(), scopeId);
        ctx.popScope(scopeId);
    } catch (ErrorInfoHolder* error) {
        return Status::Error(error->getMessage(), error->getPosition());
    }

    return Status::Ok();
}
