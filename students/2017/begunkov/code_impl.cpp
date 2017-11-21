#include "code_impl.hpp"
#include "utils.hpp"

using namespace mathvm;
using namespace std;
#define VAR_COUNT 4

void InterpreterCodeImpl::disassemble(ostream& out, FunctionFilter* filter)
{
    for (FunctionIterator it(this); it.hasNext(); ) {
        auto *foo = dynamic_cast<BytecodeFunction*>(it.next());
        if (filter == nullptr || filter->matches(foo))
            foo->disassemble(out);
    }
}

namespace mathvm {
struct Executer
{
    InterpreterCodeImpl *ctx = nullptr;

    stack<uint32_t> instructions;
    stack<Bytecode*> bytecodes;
    stack<BytecodeFunction*> foos;

    MStack<LVar> st;
    Bytecode *bc = nullptr;
    uint32_t ip = 0;
    LVar regvs[VAR_COUNT];

    #define PROCESS(b, s, l) void PR_##b();
    FOR_BYTECODES(PROCESS)
    #undef PROCESS

    void enterFunction(BytecodeFunction *foo) {
        instructions.push(ip);
        bytecodes.push(bc);
        foos.push(foo);

        bc = foo->bytecode();
        ip = 0;

        auto& batch = ctx->vars[foo->scopeId()];
        batch.resize(batch.size() + 1);
    }

    void leaveFunction() {
        auto current = foos.top();
        auto& batch = ctx->vars[current->scopeId()];
        batch.resize(batch.size() - 1);

        ip = poptop(instructions);
        bc = poptop(bytecodes);
        foos.pop();
    }

    LVar& findVar(uint16_t id, uint16_t scope)
    {
        auto& var = ctx->vars[scope].back()[id];
        return var;
    }


    void saveVars(vector<Var*>& in)
    {
        for (auto var : in) {
            auto res = ctx->varNames.find(var->name());
            if (res == ctx->varNames.end())
                continue;

            uint16_t id = res->second;
            auto& st = findVar(id, 0);
            st.set(*var);
        }
    }

    void restoreVars(vector<Var*>& in)
    {
        for (auto var : in) {
            auto res = ctx->varNames.find(var->name());
            if (res == ctx->varNames.end())
                continue;

            uint16_t id = res->second;
            auto& st = findVar(id, 0);
            st.propagate(*var);
        }
    }

    Status* execute(vector<Var*>& in)
    {
        auto main = (BytecodeFunction*)ctx->functionById(0);
        saveVars(in);

        enterFunction(main);

        try {
            while (ip < bc->length()) {
                auto ins = bc->getInsn(ip++);
                switch (ins) {
                    #define CASE_INS(b, s, l) case BC_##b: PR_##b(); break;
                    FOR_BYTECODES(CASE_INS)
                    #undef CASE_INS
                    default: break;
                }
            }
        } catch (const char *e) {
            return Status::Error(e, ip);
        }

        restoreVars(in);
        return Status::Ok();
    }
}; // ! Executer


#define GET_VAR(bc, ip, type) ({ \
    auto retval = (bc)->get##type((ip)); \
    (ip) += sizeof(retval); \
    retval; \
})

Status* InterpreterCodeImpl::execute(vector<Var*>& vars)
{
    Executer e;
    e.ctx = this;
    return e.execute(vars);
}

void Executer::PR_INVALID()
{
    throw "error";
}

void Executer::PR_DLOAD()
{
    LVar val(GET_VAR(bc, ip, Double));
    st.push(val);
}

void Executer::PR_ILOAD()
{
    LVar val(GET_VAR(bc, ip, Int64));
    st.push(val);
}

void Executer::PR_SLOAD()
{
    auto strId = GET_VAR(bc, ip, UInt16);
    auto str = ctx->constantById(strId).c_str();
    st.push(str);
}

void Executer::PR_DLOAD0()
{
    st.emplace(0.0);
}

void Executer::PR_ILOAD0()
{
    st.emplace((int64_t)0);
}

void Executer::PR_SLOAD0()
{
    auto str = ctx->constantById(0).c_str();
    st.emplace(str);
}

void Executer::PR_DLOAD1()
{
    st.emplace(1.0);
}

void Executer::PR_ILOAD1()
{
    st.emplace((int64_t)1);
}

void Executer::PR_DLOADM1()
{
    st.emplace(-1.0);
}

void Executer::PR_ILOADM1()
{
    st.emplace((int64_t)-1);
}

void Executer::PR_DADD()
{
    auto l = poptop(st);
    auto r = poptop(st);
    l.d += r.d;
    st.push(l);
}

void Executer::PR_IADD()
{
    auto l = poptop(st);
    auto r = poptop(st);
    l.i += r.i;
    st.push(l);
}

void Executer::PR_DSUB()
{
    auto l = poptop(st);
    auto r = poptop(st);
    l.d -= r.d;
    st.push(l);
}

void Executer::PR_ISUB()
{
    auto l = poptop(st);
    auto r = poptop(st);
    l.i -= r.i;
    st.push(l);
}

void Executer::PR_DMUL()
{
    auto l = poptop(st);
    auto r = poptop(st);
    l.d *= r.d;
    st.push(l);
}

void Executer::PR_IMUL()
{
    auto l = poptop(st);
    auto r = poptop(st);
    l.i *= r.i;
    st.push(l);
}

void Executer::PR_DDIV()
{
    auto l = poptop(st);
    auto r = poptop(st);
    l.d /= r.d;
    st.push(l);
}

void Executer::PR_IDIV()
{
    auto l = poptop(st);
    auto r = poptop(st);
    l.i /= r.i;
    st.push(l);
}

void Executer::PR_IMOD()
{
    auto l = poptop(st);
    auto r = poptop(st);
    l.i %= r.i;
    st.push(l);
}

void Executer::PR_DNEG()
{
    auto l = poptop(st);
    l.d *= -1;
    st.push(l);
}

void Executer::PR_INEG()
{
    auto l = poptop(st);
    l.i *= -1;
    st.push(l);
}

void Executer::PR_IAOR()
{
    auto l = poptop(st);
    auto r = poptop(st);
    l.i |= r.i;
    st.push(l);
}

void Executer::PR_IAAND()
{
    auto l = poptop(st);
    auto r = poptop(st);
    l.i &= r.i;
    st.push(l);
}

void Executer::PR_IAXOR()
{
    auto l = poptop(st);
    auto r = poptop(st);
    l.i ^= r.i;
    st.push(l);
}

void Executer::PR_IPRINT()
{
    cout << poptop(st).i;
}

void Executer::PR_DPRINT()
{
    cout << poptop(st).d;
}

void Executer::PR_SPRINT()
{
    cout << poptop(st).s;
}

void Executer::PR_I2D()
{
    st.emplace((double)poptop(st).i);
}

void Executer::PR_D2I()
{
    st.emplace((int64_t)poptop(st).d);
}

void Executer::PR_S2I()
{
    auto v = poptop(st);
    st.push((int64_t)atoi(v.s));
}

void Executer::PR_SWAP()
{
    auto l = poptop(st);
    auto r = poptop(st);

    st.push(l);
    st.push(r);
}

void Executer::PR_POP()
{
    st.pop();
}

void Executer::PR_LOADDVAR0()
{
    st.push(regvs[0]);
}

void Executer::PR_LOADDVAR1()
{
    st.push(regvs[1]);
}

void Executer::PR_LOADDVAR2()
{
    st.push(regvs[2]);
}

void Executer::PR_LOADDVAR3()
{
    st.push(regvs[3]);
}

void Executer::PR_LOADIVAR0() {PR_LOADDVAR0();}
void Executer::PR_LOADIVAR1() {PR_LOADDVAR1();}
void Executer::PR_LOADIVAR2() {PR_LOADDVAR2();}
void Executer::PR_LOADIVAR3() {PR_LOADDVAR3();}
void Executer::PR_LOADSVAR0() {PR_LOADDVAR0();}
void Executer::PR_LOADSVAR1() {PR_LOADDVAR1();}
void Executer::PR_LOADSVAR2() {PR_LOADDVAR2();}
void Executer::PR_LOADSVAR3() {PR_LOADDVAR3();}

void Executer::PR_STOREDVAR0()
{
    regvs[0] = poptop(st);
}

void Executer::PR_STOREDVAR1()
{
    regvs[1] = poptop(st);
}

void Executer::PR_STOREDVAR2()
{
    regvs[2] = poptop(st);
}

void Executer::PR_STOREDVAR3()
{
    regvs[3] = poptop(st);
}

void Executer::PR_STOREIVAR0() {PR_STOREDVAR0();}
void Executer::PR_STOREIVAR1() {PR_STOREDVAR1();}
void Executer::PR_STOREIVAR2() {PR_STOREDVAR2();}
void Executer::PR_STOREIVAR3() {PR_STOREDVAR3();}
void Executer::PR_STORESVAR0() {PR_STOREDVAR0();}
void Executer::PR_STORESVAR1() {PR_STOREDVAR1();}
void Executer::PR_STORESVAR2() {PR_STOREDVAR2();}
void Executer::PR_STORESVAR3() {PR_STOREDVAR3();}

void Executer::PR_LOADDVAR()
{
    auto id = GET_VAR(bc, ip, UInt16);
    assert(id < VAR_COUNT);

    st.push(regvs[id]);
}

void Executer::PR_LOADIVAR()
{
    PR_LOADDVAR();
}

void Executer::PR_LOADSVAR()
{
    PR_LOADDVAR();
}

void Executer::PR_STOREDVAR()
{
    LVar val = poptop(st);

    auto id = GET_VAR(bc, ip, UInt16);
    assert(id < VAR_COUNT);

    regvs[id] = val;
}

void Executer::PR_STOREIVAR()
{
    PR_STOREDVAR();
}

void Executer::PR_STORESVAR()
{
    PR_STOREDVAR();
}

void Executer::PR_LOADCTXDVAR()
{
    auto scope = GET_VAR(bc, ip, UInt16);
    auto varid = GET_VAR(bc, ip, UInt16);
    auto& var = findVar(varid, scope);
    st.push(var);
}

void Executer::PR_LOADCTXIVAR()
{
    PR_LOADCTXDVAR();
}

void Executer::PR_LOADCTXSVAR()
{
    PR_LOADCTXDVAR();
}

void Executer::PR_STORECTXDVAR()
{
    auto scope = GET_VAR(bc, ip, UInt16);
    auto varid = GET_VAR(bc, ip, UInt16);
    auto& var = findVar(varid, scope);
    var = poptop(st);
}

void Executer::PR_STORECTXIVAR()
{
    PR_STORECTXDVAR();
}

void Executer::PR_STORECTXSVAR()
{
    PR_STORECTXDVAR();
}

void Executer::PR_DCMP()
{
    LVar l = poptop(st);
    LVar r = poptop(st);

    int64_t res =  (l.d == r.d) ? 0 :
                    (l.d < r.d) ? -1: 1;
    st.push(res);
}

void Executer::PR_ICMP()
{
    LVar l = poptop(st);
    LVar r = poptop(st);

    int64_t res =   (l.i == r.i) ? 0 :
                    (l.i < r.i) ? -1
                                : 1;
    st.push(res);
}

void Executer::PR_JA()
{
    auto off = bc->getInt16(ip);
    ip += off;
}

void Executer::PR_IFICMPNE()
{
    auto l = poptop(st);
    auto r = poptop(st);

    auto tj = GET_VAR(bc, ip, Int16);
    if (l.i != r.i)
        ip += tj - sizeof(tj);
}

void Executer::PR_IFICMPE()
{
    auto l = poptop(st);
    auto r = poptop(st);

    auto tj = GET_VAR(bc, ip, Int16);
    if (l.i == r.i)
        ip += tj - sizeof(tj);
}

void Executer::PR_IFICMPG()
{
    auto l = poptop(st);
    auto r = poptop(st);

    auto tj = GET_VAR(bc, ip, Int16);
    if (l.i > r.i)
        ip += tj - sizeof(tj);
}

void Executer::PR_IFICMPGE()
{
    auto l = poptop(st);
    auto r = poptop(st);

    auto tj = GET_VAR(bc, ip, Int16);
    if (l.i >= r.i)
        ip += tj - sizeof(tj);
}

void Executer::PR_IFICMPL()
{
    auto l = poptop(st);
    auto r = poptop(st);

    auto tj = GET_VAR(bc, ip, Int16);
    if (l.i < r.i)
        ip += tj - sizeof(tj);
}

void Executer::PR_IFICMPLE()
{
    auto l = poptop(st);
    auto r = poptop(st);

    auto tj = GET_VAR(bc, ip, Int16);
    if (l.i <= r.i)
        ip += tj - sizeof(tj);
}

void Executer::PR_DUMP()
{
    auto l = st.top();
    cout << "dump: i " << l.i <<
            " d " << l.d <<
            " *s " << (size_t)l.s
            << endl;
}

void Executer::PR_STOP()
{
    ip = bc->length();
}

void Executer::PR_CALL()
{
    auto id = GET_VAR(bc, ip, UInt16);
    auto foo = (BytecodeFunction*)ctx->functionById(id);
    enterFunction(foo);
}

void Executer::PR_CALLNATIVE()
{
    auto id = GET_VAR(bc, ip, UInt16);

//    void *stack = malloc(16 * 1000);

    const Signature *signature;
    const string *name;
    const void *ex = ctx->nativeById(id, &signature, &name);

    size_t sz = signature->size() - 1;
    size_t fst_idx = st.size() - sz;

    size_t regsz = 0;
    intptr_t regs[max(sz, (size_t)8)] = {};
    size_t flssz = 0;
    double fls[max(sz, (size_t)8)] = {};

    for (size_t i = 1; i < signature->size(); ++i) {
        if ((*signature)[i].first == VT_DOUBLE)
            fls[flssz++] = st[fst_idx++].d;
        else
            regs[regsz++] = (intptr_t)(st[fst_idx++].i);
    }

    for (size_t i = 1; i < signature->size(); ++i)
        st.pop();

    size_t resi = 1;
    double resd = 0.0;

    asm volatile (
        "mov %1, %%r14;"
        "movsd 0(%2), %%xmm0;"
        "movsd 8(%2), %%xmm1;"
        "movsd 16(%2), %%xmm2;"
        "movsd 24(%2), %%xmm3;"
        "movsd 32(%2), %%xmm4;"
        "movsd 40(%2), %%xmm5;"
        "movsd 48(%2), %%xmm6;"
        "movsd 56(%2), %%xmm7;"
        "mov 0(%3), %%rdi;"
        "mov 8(%3), %%rsi;"
        "mov 16(%3), %%rdx;"
        "mov 24(%3), %%rcx;"
        "mov 32(%3), %%r8;"
        "mov 40(%3), %%r9;"
        "call %4;"
        "mov %%rax, %0;"
        "movsd %%xmm0, 0(%%r14);"
        : "=r" (resi)
        : "r" (&resd), "r" (fls), "r" (regs), "r" (ex)
        : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
            "rdi", "rsi", "rdx", "rcx", "r8", "r8", "rax", "rbx", "r14"
    );


    auto ret = signature[0][0].first;
    if (ret == VT_INT || ret == VT_STRING) {
        st.push(LVar((int64_t)resi));
    } else if (ret == VT_DOUBLE) {
        st.push(LVar(resd));
    }
}

void Executer::PR_RETURN()
{
    leaveFunction();
}

void Executer::PR_BREAK()
{
    PR_STOP();
}

}
