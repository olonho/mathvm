#include "code_impl.hpp"
#include <limits.h>

using namespace mathvm;
using namespace std;

template <typename T>
static T poptop(stack<T>& container)
{
    assert(!container.empty());

    T res = container.top();
    container.pop();
    return res;
}

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
    stack<LVar> st;
    Bytecode *bc = nullptr;
    uint32_t ip = 0;

    LVar v0;
    int gen = 0;

    #define PROCESS(b, s, l) void PR_##b();
    FOR_BYTECODES(PROCESS)
    #undef PROCESS

    LVar& findVar(uint16_t id) {
        using VarID = InterpreterCodeImpl::VarID;
        auto& var = ctx->vars[VarID(id, gen)];
        return var;
    }


    void saveVars(vector<Var*>& in) {
        using VarID = InterpreterCodeImpl::VarID;
        for (auto var : in) {
            auto res = ctx->varNames.find(var->name());
            if (res == ctx->varNames.end())
                continue;

            uint16_t id = res->second;
            auto& st = ctx->vars[VarID(id, 0)];

            switch (var->type()) {
                case VT_INT: st.i = var->getIntValue(); break;
                case VT_DOUBLE: st.d = var->getDoubleValue(); break;
                case VT_STRING: st.s = var->getStringValue(); break;
                default:;
            }
        }
    }

    void restoreVars(vector<Var*>& in) {
        using VarID = InterpreterCodeImpl::VarID;
        for (auto var : in) {
            auto res = ctx->varNames.find(var->name());
            if (res == ctx->varNames.end())
                continue;

            uint16_t id = res->second;
            auto& st = ctx->vars[VarID(id, 0)];

            switch (var->type()) {
                case VT_INT: var->setIntValue(st.i); break;
                case VT_DOUBLE: var->setDoubleValue(st.d); break;
                case VT_STRING: var->setStringValue(st.s); break;
                default:;
            }
        }
    }

    Status* execute(vector<Var*>& in)
    {
        auto main = (BytecodeFunction*)ctx->functionById(0);
        saveVars(in);

        bytecodes.push(main->bytecode());
        instructions.push(0);

        bc = bytecodes.top();
        ip = 0;

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

void Executer::PR_LOADDVAR0() {PR_INVALID();}
void Executer::PR_LOADDVAR1() {PR_INVALID();}
void Executer::PR_LOADDVAR2() {PR_INVALID();}
void Executer::PR_LOADDVAR3() {PR_INVALID();}
void Executer::PR_LOADIVAR0() {
    st.push(v0);
}
void Executer::PR_LOADIVAR1() {PR_INVALID();}
void Executer::PR_LOADIVAR2() {PR_INVALID();}
void Executer::PR_LOADIVAR3() {PR_INVALID();}
void Executer::PR_LOADSVAR0() {PR_INVALID();}
void Executer::PR_LOADSVAR1() {PR_INVALID();}
void Executer::PR_LOADSVAR2() {PR_INVALID();}
void Executer::PR_LOADSVAR3() {PR_INVALID();}
void Executer::PR_STOREDVAR0() {PR_INVALID();}
void Executer::PR_STOREDVAR1() {PR_INVALID();}
void Executer::PR_STOREDVAR2() {PR_INVALID();}
void Executer::PR_STOREDVAR3() {PR_INVALID();}
void Executer::PR_STOREIVAR0() {
    v0 = poptop(st);
}
void Executer::PR_STOREIVAR1() {PR_INVALID();}
void Executer::PR_STOREIVAR2() {PR_INVALID();}
void Executer::PR_STOREIVAR3() {PR_INVALID();}
void Executer::PR_STORESVAR0() {PR_INVALID();}
void Executer::PR_STORESVAR1() {PR_INVALID();}
void Executer::PR_STORESVAR2() {PR_INVALID();}
void Executer::PR_STORESVAR3() {PR_INVALID();}

void Executer::PR_LOADDVAR()
{
//    auto id = GET_VAR(bc, ip, UInt16);
//    st.push(ctx->vars[id]);
}

void Executer::PR_LOADIVAR()
{
//    auto id = GET_VAR(bc, ip, UInt16);
//    st.push(ctx->vars[id]);
}

void Executer::PR_LOADSVAR()
{
//    auto id = GET_VAR(bc, ip, UInt16);
//    st.push(ctx->vars[id]);
}

void Executer::PR_STOREDVAR()
{
//    LVar val = poptop(st);
//    auto id = GET_VAR(bc, ip, UInt16);
//    ctx->vars[id] = val;
}

void Executer::PR_STOREIVAR()
{
//    LVar val = poptop(st);
//    auto id = GET_VAR(bc, ip, UInt16);
//    ctx->vars[id] = val;
}

void Executer::PR_STORESVAR()
{
}

void Executer::PR_LOADCTXDVAR()
{
    (void)GET_VAR(bc, ip, UInt16);
    auto varid = GET_VAR(bc, ip, UInt16);
    auto& var = findVar(varid);
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
    (void)GET_VAR(bc, ip, UInt16);
    auto varid = GET_VAR(bc, ip, UInt16);
    auto& var = findVar(varid);
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
    ++gen;

    auto foo = (BytecodeFunction*)ctx->functionById(id);
    instructions.push(ip);
    bytecodes.push(bc);

    bc = foo->bytecode();
    ip = 0;
}

void Executer::PR_CALLNATIVE()
{
}

void Executer::PR_RETURN()
{
    --gen;
    bc = poptop(bytecodes);
    ip = poptop(instructions);
}

void Executer::PR_BREAK()
{
    PR_STOP();
}

}
