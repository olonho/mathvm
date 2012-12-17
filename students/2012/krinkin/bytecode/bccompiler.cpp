#include <iostream>
#include <map>

#include <cassert>

#include "bccompiler.h"

void * allocate_locals(BCCompiler *cc, size_t id, size_t size)
{
    cc->push_locals(id, size);
    return (void *)cc->locals(id);
}

GPVar gen_alloc(Compiler *cc, BCCompiler *bc, BytecodeFunction *f)
{
    GPVar ret(cc->newGP());
    GPVar bcptr(cc->newGP());
    cc->mov(bcptr, imm((size_t)bc));
    GPVar id(cc->newGP());
    cc->mov(id, imm((size_t)f->id()));
    GPVar size(cc->newGP());
    cc->mov(size, imm((size_t)f->localsNumber()));
    ECall *call = cc->call(imm((size_t)&allocate_locals));
    call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder3<size_t, size_t, size_t, size_t>());
    call->setArgument(0, bcptr);
    call->setArgument(1, id);
    call->setArgument(2, size);
    call->setReturn(ret);
    cc->unuse(bcptr);
    cc->unuse(id);
    cc->unuse(size);
    return ret;
}

void free_locals(BCCompiler *cc, size_t id)
{
    cc->pop_locals(id);
}

void gen_free(Compiler *cc, BCCompiler *bc, BytecodeFunction *f)
{
    GPVar bcptr(cc->newGP());
    cc->mov(bcptr, imm((size_t)bc));
    GPVar id(cc->newGP());
    cc->mov(id, imm((size_t)f->id()));
    ECall *call = cc->call(imm((size_t)&free_locals));
    call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<void, size_t, size_t>());
    call->setArgument(0, bcptr);
    call->setArgument(1, id);
    cc->unuse(bcptr);
    cc->unuse(id);
}

void *get_locals(BCCompiler *bc, size_t id)
{
    return bc->locals(id);
}

GPVar gen_get(Compiler *cc, BCCompiler *bc, size_t id)
{
    GPVar bcptr(cc->newGP());
    cc->mov(bcptr, imm((size_t)bc));
    GPVar idv(cc->newGP());
    cc->mov(idv, imm(id));
    GPVar ret(cc->newGP());
    ECall *call = cc->call(imm((size_t)&get_locals));
    call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<size_t, size_t, size_t>());
    call->setArgument(0, bcptr);
    call->setArgument(1, idv);
    call->setReturn(ret);
    cc->unuse(bcptr);
    cc->unuse(idv);
    return ret;
}

void load_double_from_scope(GPVar &base, size_t offset, Compiler *cc, std::stack<var_t> *vars)
{
    var_t d(cc->newXMM());
    cc->movq(d.xmm, qword_ptr(base, offset));
    vars->push(d);
}

void store_double_to_scope(GPVar &base, size_t offset, Compiler *cc, std::stack<var_t> *vars)
{
    var_t d = vars->top(); vars->pop();
    GPVar tmp(cc->newGP());
    cc->movq(tmp, d.xmm);
    cc->mov(qword_ptr(base, offset), tmp);
    cc->unuse(d.xmm);
    cc->unuse(tmp);
}

void load_int_from_scope(GPVar &base, size_t offset, Compiler *cc, std::stack<var_t> *vars)
{
    var_t i(cc->newGP());
    cc->mov(i.gp, qword_ptr(base, offset));
    vars->push(i);
}

void store_int_to_scope(GPVar &base, size_t offset, Compiler *cc, std::stack<var_t> *vars)
{
    var_t i = vars->top(); vars->pop();
    cc->mov(qword_ptr(base, offset), i.gp);
    cc->unuse(i.gp);
}

void load_string_from_scope(GPVar &base, size_t offset, Compiler *cc, std::stack<var_t> *vars)
{
    var_t s(cc->newGP());
    cc->mov(s.gp, qword_ptr(base, offset));
    vars->push(s);
}

void store_string_to_scope(GPVar &base, size_t offset, Compiler *cc, std::stack<var_t> *vars)
{
    var_t s = vars->top(); vars->pop();
    cc->mov(qword_ptr(base, offset), s.gp);
    cc->unuse(s.gp);
}

void print_int(int64_t value)
{
    std::cout << value;
}

void gen_print_int(Compiler *cc, std::stack<var_t> *vars)
{
    var_t v = vars->top(); vars->pop();
    ECall *call = cc->call(imm((size_t)&print_int));
    call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int64_t>());
    call->setArgument(0, v.gp);
    cc->unuse(v.gp);
}

void print_string(char const * value)
{
    std::cout << value;
}

void gen_print_string(Compiler *cc, std::stack<var_t> *vars)
{
    var_t v = vars->top(); vars->pop();
    ECall *call = cc->call(imm((size_t)&print_string));
    call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, size_t>());
    call->setArgument(0, v.gp);
    cc->unuse(v.gp);
}

void print_double(double value)
{
    std::cout << value;
}

void gen_print_double(Compiler *cc, std::stack<var_t> *vars)
{
    var_t v = vars->top(); vars->pop();
    ECall *call = cc->call(imm((size_t)&print_double));
    call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, double>());
    call->setArgument(0, v.xmm);
    cc->unuse(v.xmm);
}

double from_int(int64_t i)
{
    return (double)i;
}

void gen_int2double(Compiler *cc, std::stack<var_t> *vars)
{
    var_t in = vars->top(); vars->pop();
    ECall *call = cc->call(imm((size_t)&from_int));
    call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<double, int64_t>());
    call->setArgument(0, in.gp);
    var_t ret(cc->newXMM());
    call->setReturn(ret.xmm);
    cc->unuse(in.gp);
    vars->push(ret);
}

int64_t from_double(double d)
{
    return (int64_t)d;
}

void gen_double2int(Compiler *cc, std::stack<var_t> *vars)
{
    var_t in = vars->top(); vars->pop();
    ECall *call = cc->call(imm((size_t)&from_double));
    call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<int64_t, double>());
    call->setArgument(0, in.xmm);
    var_t ret(cc->newGP());
    call->setReturn(ret.gp);
    cc->unuse(in.xmm);
    vars->push(ret);
}

void load_value(int64_t value, Compiler *cc, std::stack<var_t> *vars)
{
    var_t v(cc->newGP());
    cc->mov(v.gp, imm(value));
    vars->push(v);
    
}

void load_value(char const *value, Compiler *cc, std::stack<var_t> *vars)
{
    var_t v(cc->newGP());
    cc->mov(v.gp, imm((size_t)value));
    vars->push(v);
}

void load_value(double value, Compiler *cc, std::stack<var_t> *vars)
{
    GPVar tmp(cc->newGP());
    cc->mov(tmp, imm(*((int64_t*)&value)));
    var_t v(cc->newXMM());
    cc->movq(v.xmm, tmp);
    cc->unuse(tmp);
    vars->push(v);
}

void load_double_var(size_t index, Compiler *cc, BCCompiler *bc, std::stack<var_t> *vars)
{
    GPVar tmp(cc->newGP());
    cc->mov(tmp, imm((size_t)(bc->m_dvars + index)));
    var_t dvar(cc->newXMM());
    cc->movq(dvar.xmm, qword_ptr(tmp));
    cc->unuse(tmp);
    vars->push(dvar);
}

void store_double_var(size_t index, Compiler *cc, BCCompiler *bc, std::stack<var_t> *vars)
{
    GPVar tmp(cc->newGP());
    cc->mov(tmp, imm((size_t)(bc->m_dvars + index)));
    var_t d = vars->top(); vars->pop();
    cc->movq(dword_ptr(tmp), d.xmm);
    cc->unuse(d.xmm);
    cc->unuse(tmp);
}

void load_int_var(size_t index, Compiler *cc, BCCompiler *bc, std::stack<var_t> *vars)
{
    GPVar tmp(cc->newGP());
    cc->mov(tmp, imm((size_t)(bc->m_ivars + index)));
    var_t ivar(cc->newGP());
    cc->mov(ivar.gp, qword_ptr(tmp));
    cc->unuse(tmp);
    vars->push(ivar);
}

void store_int_var(size_t index, Compiler *cc, BCCompiler *bc, std::stack<var_t> *vars)
{
    GPVar tmp(cc->newGP());
    cc->mov(tmp, imm((size_t)(bc->m_ivars + index)));
    var_t i = vars->top(); vars->pop();
    cc->mov(dword_ptr(tmp), i.gp);
    cc->unuse(i.gp);
    cc->unuse(tmp);
}

void load_string_var(size_t index, Compiler *cc, BCCompiler *bc, std::stack<var_t> *vars)
{
    GPVar tmp(cc->newGP());
    cc->mov(tmp, imm((size_t)(bc->m_svars + index)));
    var_t svar(cc->newGP());
    cc->mov(svar.gp, qword_ptr(tmp));
    cc->unuse(tmp);
    vars->push(svar);
}

void store_string_var(size_t index, Compiler *cc, BCCompiler *bc, std::stack<var_t> *vars)
{
    GPVar tmp(cc->newGP());
    cc->mov(tmp, imm((size_t)(bc->m_svars + index)));
    var_t s = vars->top(); vars->pop();
    cc->mov(dword_ptr(tmp), s.gp);
    cc->unuse(s.gp);
    cc->unuse(tmp);
}

int64_t dcmp_impl(double d1, double d2)
{
    if (d1 < d2) return (int64_t)-1;
    else if (d1 > d2) return (int64_t)1;
    else return (int64_t)0;
}

int64_t idiv(int64_t n, int64_t d)
{
    return n/d;
}

void gen_idiv(Compiler *cc, std::stack<var_t> *vars)
{
    var_t upper = vars->top(); vars->pop();
    var_t lower = vars->top(); vars->pop();
    ECall *call = cc->call((size_t)&idiv);
    call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, int64_t, int64_t>());
    var_t res(cc->newGP());
    call->setArgument(0, upper.gp);
    cc->unuse(upper.gp);
    call->setArgument(1, lower.gp);
    cc->unuse(lower.gp);
    call->setReturn(res.gp);
    vars->push(res);
}

int64_t imod(int64_t n, int64_t d)
{
    return n%d;
}

void gen_imod(Compiler *cc, std::stack<var_t> *vars)
{
    var_t upper = vars->top(); vars->pop();
    var_t lower = vars->top(); vars->pop();
    ECall *call = cc->call((size_t)&imod);
    call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, int64_t, int64_t>());
    var_t res(cc->newGP());
    call->setArgument(0, upper.gp);
    cc->unuse(upper.gp);
    call->setArgument(1, lower.gp);
    cc->unuse(lower.gp);
    call->setReturn(res.gp);
    vars->push(res);
}

void gen_dcmp(Compiler *cc, std::stack<var_t> *vars)
{
    var_t upper = vars->top(); vars->pop();
    var_t lower = vars->top(); vars->pop();
    var_t ret(cc->newXMM());
    ECall *call = cc->call(imm((size_t)&dcmp_impl));
    call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, double, double>());
    call->setArgument(0, upper.xmm);
    call->setArgument(1, lower.xmm);
    call->setReturn(ret.xmm);
    cc->unuse(upper.xmm);
    cc->unuse(lower.xmm);
    vars->push(ret);
}

void gen_load_double_arg(GPVar frame, size_t offset, Compiler *cc, std::stack<var_t> *vars)
{
    var_t r(cc->newXMM());
    cc->movq(r.xmm, qword_ptr(frame, offset));
    vars->push(r);
}

void gen_load_string_arg(GPVar frame, size_t offset, Compiler *cc, std::stack<var_t> *vars)
{
    var_t r(cc->newGP());
    cc->mov(r.gp, qword_ptr(frame, offset));
    vars->push(r);
}

void gen_load_int_arg(GPVar frame, size_t offset, Compiler *cc, std::stack<var_t> *vars)
{
    var_t r(cc->newGP());
    cc->mov(r.gp, qword_ptr(frame, offset));
    vars->push(r);
}

BCCompiler::BCCompiler(BCCode *code)
    : m_code(code)
{
    size_t functions = m_code->function_count();
    m_locals.resize(functions);
    m_fptrs.resize(functions);
    
    for (size_t id = 0; id != functions; ++id)
        generate((BytecodeFunction *)m_code->functionById(id));
}

BCCompiler::~BCCompiler()
{
    for (size_t id = 0; id != m_fptrs.size(); ++id)
        MemoryManager::getGlobal()->free((void*)(m_fptrs[id]));
}

void BCCompiler::execute()
{
    m_fptrs[0]();
}

void BCCompiler::generate(BytecodeFunction *f)
{
    Bytecode *bcode = f->bytecode();

    Compiler cc;
    FileLogger logger(stdout);
    cc.setLogger(&logger);
    
    std::stack<var_t> vars;
    std::map<size_t, AsmJit::Label> bind;

    cc.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<void>());
    
    AsmJit::Label exit = cc.newLabel();

    GPVar locals = gen_alloc(&cc, this, f);

    size_t bci = 0, offset = 0;
    Instruction insn = bcode->getInsn(bci);
    GPVar frame = cc.newGP();
    cc.mov(frame, imm((size_t)m_frame));
    while ((insn == BC_STOREIVAR) || (insn == BC_STORESVAR) || (insn == BC_STOREDVAR))
    {
        if (insn == BC_STOREIVAR)
        {
            gen_load_int_arg(frame, offset, &cc, &vars);
            store_int_to_scope(locals, (size_t)bcode->getUInt16(bci + 1), &cc, &vars);
            offset += sizeof(char const *);
        }
        else if (insn == BC_STORESVAR)
        {
            gen_load_string_arg(frame, offset, &cc, &vars);
            store_string_to_scope(locals, (size_t)bcode->getUInt16(bci + 1), &cc, &vars);
            offset += sizeof(int64_t);
        }
        else
        {
            gen_load_double_arg(frame, offset, &cc, &vars);
            store_double_to_scope(locals, (size_t)bcode->getUInt16(bci + 1), &cc, &vars);
            offset += sizeof(double);
        }
        bci += m_code->insn_size(insn);
        insn = bcode->getInsn(bci);
    }
    cc.unuse(frame);

    for (; bci != bcode->length();)
    {
        uint8_t flags = m_code->annotation(f->id(), bci);
        
        if (flags & SAVE_RESULT)
        {
            var_t upper = vars.top(); vars.pop();
            var_t lower = vars.top();
            if (upper.kind)
            {
                cc.mov(lower.gp, upper.gp);
                cc.unuse(upper.gp);
            }
            else
            {
                cc.movq(lower.xmm, upper.xmm);
                cc.unuse(upper.xmm);
            }
        }
        if (flags & UNLOCK_RESULT) vars.top().locked = false;
        if (flags & LOCK_RESULT) vars.top().locked = true;
        
        std::map<size_t, AsmJit::Label>::iterator it = bind.find(bci);
        if (it == bind.end()) bind[bci] = cc.newLabel();
        cc.bind(bind[bci]);
        
        insn = bcode->getInsn(bci);

        switch (insn)
        {
        case BC_DLOAD:
            load_value(bcode->getDouble(bci + 1), &cc, &vars);
            break;
        case BC_ILOAD:
            load_value(bcode->getInt64(bci + 1), &cc, &vars);
            break;
        case BC_SLOAD:
            load_value(m_code->constantById(bcode->getUInt16(bci + 1)).c_str(), &cc, &vars);
            break;
        case BC_DLOAD0:
            load_value(0.0, &cc, &vars);
            break;
        case BC_ILOAD0:
            load_value((int64_t)0, &cc, &vars);
            break;
        case BC_SLOAD0:
            load_value(m_empty.c_str(), &cc, &vars);
            break;
        case BC_DLOAD1:
            load_value(1.0, &cc, &vars);
            break;
        case BC_ILOAD1:
            load_value((int64_t)1, &cc, &vars);
            break;
        case BC_DLOADM1:
            load_value(-1.0, &cc, &vars);
            break;
        case BC_ILOADM1:
            load_value((int64_t)-1, &cc, &vars);
            break;
        case BC_DADD:
            {
                var_t upper = vars.top(); vars.pop();
                var_t lower = vars.top();
                cc.addsd(lower.xmm, upper.xmm);
                cc.unuse(upper.xmm);
            }
            break;
        case BC_DSUB:
            {
                var_t upper = vars.top(); vars.pop();
                var_t lower = vars.top(); vars.pop();
                cc.subsd(upper.xmm, lower.xmm);
                cc.unuse(lower.xmm);
                vars.push(upper);
            }
            break;
        case BC_DMUL:
            {
                var_t upper = vars.top(); vars.pop();
                var_t lower = vars.top();
                cc.mulsd(lower.xmm, upper.xmm);
                cc.unuse(upper.xmm);
            }
            break;
        case BC_DDIV:
            {
                var_t upper = vars.top(); vars.pop();
                var_t lower = vars.top(); vars.pop();
                cc.divsd(upper.xmm, lower.xmm);
                cc.unuse(lower.xmm);
                vars.push(upper);
            }
            break;
        case BC_IADD:
            {
                var_t upper = vars.top(); vars.pop();
                var_t lower = vars.top();
                cc.add(lower.gp, upper.gp);
                cc.unuse(upper.gp);
            }
            break;
        case BC_ISUB:
            {
                var_t upper = vars.top(); vars.pop();
                var_t lower = vars.top(); vars.pop();
                cc.sub(upper.gp, lower.gp);
                cc.unuse(lower.gp);
                vars.push(upper);
            }
            break;
        case BC_IMUL:
            {
                var_t upper = vars.top(); vars.pop();
                var_t lower = vars.top();
                cc.imul(lower.gp, upper.gp);
                cc.unuse(upper.gp);
            }
            break;
        case BC_IDIV:
            gen_idiv(&cc, &vars);
            break;
        case BC_IMOD:
            gen_imod(&cc, &vars);
            break;
        case BC_DNEG:
            {
                load_value(0.0, &cc, &vars);
                var_t upper = vars.top(); vars.pop();
                var_t lower = vars.top(); vars.pop();
                cc.subsd(upper.xmm, lower.xmm);
                cc.unuse(lower.xmm);
                vars.push(upper);
            }
            break;
        case BC_INEG:
            {
                var_t value = vars.top();
                cc.neg(value.gp);
            }
            break;
        case BC_IPRINT:
            gen_print_int(&cc, &vars);
            break;
        case BC_DPRINT:
            gen_print_double(&cc, &vars);
            break;
        case BC_SPRINT:
            gen_print_string(&cc, &vars);
            break;
        case BC_I2D:
            gen_int2double(&cc, &vars);
            break;
        case BC_D2I:
            gen_double2int(&cc, &vars);
            break;
        case BC_S2I:
            break;
        case BC_SWAP:
            {
                /*
                var_t upper = vars.top(); vars.pop();
                var_t lower = vars.top(); vars.pop();
                vars.push(upper);
                vars.push(lower);
                */
            }
            break;
        case BC_POP:
            {
                var_t upper = vars.top();
                
                if (!upper.locked)
                {
                    vars.pop();
                    if (upper.kind) cc.unuse(upper.gp);
                    else cc.unuse(upper.xmm);
                }
            }
            break;
        case BC_LOADDVAR0:
            load_double_var(0, &cc, this, &vars);
            break;
        case BC_LOADDVAR1:
            load_double_var(1, &cc, this, &vars);
            break;
        case BC_LOADDVAR2:
            load_double_var(2, &cc, this, &vars);
            break;
        case BC_LOADDVAR3:
            load_double_var(3, &cc, this, &vars);
            break;
        case BC_LOADIVAR0:
            load_int_var(0, &cc, this, &vars);
            break;
        case BC_LOADIVAR1:
            load_int_var(1, &cc, this, &vars);
            break;
        case BC_LOADIVAR2:
            load_int_var(2, &cc, this, &vars);
            break;
        case BC_LOADIVAR3:
            load_int_var(3, &cc, this, &vars);
            break;
        case BC_LOADSVAR0:
            load_string_var(0, &cc, this, &vars);
            break;
        case BC_LOADSVAR1:
            load_string_var(1, &cc, this, &vars);
            break;
        case BC_LOADSVAR2:
            load_string_var(2, &cc, this, &vars);
            break;
        case BC_LOADSVAR3:
            load_string_var(3, &cc, this, &vars);
            break;
        case BC_STOREDVAR0:
            store_double_var(0, &cc, this, &vars);
            break;
        case BC_STOREDVAR1:
            store_double_var(1, &cc, this, &vars);
            break;
        case BC_STOREDVAR2:
            store_double_var(2, &cc, this, &vars);
            break;
        case BC_STOREDVAR3:
            store_double_var(3, &cc, this, &vars);
            break;
        case BC_STOREIVAR0:
            store_int_var(0, &cc, this, &vars);
            break;
        case BC_STOREIVAR1:
            store_int_var(1, &cc, this, &vars);
            break;
        case BC_STOREIVAR2:
            store_int_var(2, &cc, this, &vars);
            break;
        case BC_STOREIVAR3:
            store_int_var(3, &cc, this, &vars);
            break;
        case BC_STORESVAR0:
            store_string_var(0, &cc, this, &vars);
            break;
        case BC_STORESVAR1:
            store_string_var(1, &cc, this, &vars);
            break;
        case BC_STORESVAR2:
            store_string_var(2, &cc, this, &vars);
            break;
        case BC_STORESVAR3:
            store_string_var(3, &cc, this, &vars);
            break;
        case BC_LOADDVAR:
            {
                size_t offset = (size_t)bcode->getUInt16(bci + 1);
                load_double_from_scope(locals, offset, &cc, &vars);
            }
            break;
        case BC_LOADIVAR:
            {
                size_t offset = (size_t)bcode->getUInt16(bci + 1);
                load_int_from_scope(locals, offset, &cc, &vars);
            }
            break;
        case BC_LOADSVAR:
            {
                size_t offset = (size_t)bcode->getUInt16(bci + 1);
                load_string_from_scope(locals, offset, &cc, &vars);
            }
            break;
        case BC_STOREDVAR:
            {
                size_t offset = (size_t)bcode->getUInt16(bci + 1);
                store_double_to_scope(locals, offset, &cc, &vars);
            }
            break;
        case BC_STOREIVAR:
            {
                size_t offset = (size_t)bcode->getUInt16(bci + 1);
                store_int_to_scope(locals, offset, &cc, &vars);
            }
            break;
        case BC_STORESVAR:
            {
                size_t offset = (size_t)bcode->getUInt16(bci + 1);
                store_string_to_scope(locals, offset, &cc, &vars);
            }
            break;
        case BC_LOADCTXDVAR:
            {
                size_t id = (size_t)bcode->getUInt16(bci + 1);
                size_t offset = (size_t)bcode->getUInt16(bci + 3);
                GPVar scope = gen_get(&cc, this, id);
                load_double_from_scope(scope, offset, &cc, &vars);
                cc.unuse(scope);
            }
            break;
        case BC_LOADCTXIVAR:
            {
                size_t id = (size_t)bcode->getUInt16(bci + 1);
                size_t offset = (size_t)bcode->getUInt16(bci + 3);
                GPVar scope = gen_get(&cc, this, id);
                load_int_from_scope(scope, offset, &cc, &vars);
                cc.unuse(scope);
            }
            break;
        case BC_LOADCTXSVAR:
            {
                size_t id = (size_t)bcode->getUInt16(bci + 1);
                size_t offset = (size_t)bcode->getUInt16(bci + 3);
                GPVar scope = gen_get(&cc, this, id);
                load_string_from_scope(scope, offset, &cc, &vars);
                cc.unuse(scope);
            }
            break;
        case BC_STORECTXDVAR:
            {
                size_t id = (size_t)bcode->getUInt16(bci + 1);
                size_t offset = (size_t)bcode->getUInt16(bci + 3);
                GPVar scope = gen_get(&cc, this, id);
                store_double_to_scope(scope, offset, &cc, &vars);
                cc.unuse(scope);
            }
            break;
        case BC_STORECTXIVAR:
            {
                size_t id = (size_t)bcode->getUInt16(bci + 1);
                size_t offset = (size_t)bcode->getUInt16(bci + 3);
                GPVar scope = gen_get(&cc, this, id);
                store_int_to_scope(scope, offset, &cc, &vars);
                cc.unuse(scope);
            }
            break;
        case BC_STORECTXSVAR:
            {
                size_t id = (size_t)bcode->getUInt16(bci + 1);
                size_t offset = (size_t)bcode->getUInt16(bci + 3);
                GPVar scope = gen_get(&cc, this, id);
                store_string_to_scope(scope, offset, &cc, &vars);
                cc.unuse(scope);
            }
            break;
        case BC_DCMP:
            gen_dcmp(&cc, &vars);
            break;
        case BC_ICMP:
            {
                var_t upper = vars.top(); vars.pop();
                var_t lower = vars.top(); vars.pop();
                cc.sub(upper.gp, lower.gp);
                vars.push(upper);
            }
            break;
        case BC_JA:
            {
                size_t addr = bci + 1 + bcode->getInt16(bci + 1);
                std::map<size_t, AsmJit::Label>::iterator it = bind.find(addr);
                if (it == bind.end()) bind[addr] = cc.newLabel();
                cc.jmp(bind[addr]);
            }
            break;
        case BC_IFICMPNE:
            {
                size_t addr = bci + 1 + bcode->getInt16(bci + 1);
                var_t upper = vars.top(); vars.pop();
                var_t lower = vars.top(); vars.push(upper);
                cc.cmp(upper.gp, lower.gp);
                std::map<size_t, AsmJit::Label>::iterator it = bind.find(addr);
                if (it == bind.end()) bind[addr] = cc.newLabel();
                cc.jne(bind[addr]);
            }
            break;
        case BC_IFICMPE:
            {
                size_t addr = bci + 1 + bcode->getInt16(bci + 1);
                var_t upper = vars.top(); vars.pop();
                var_t lower = vars.top(); vars.push(upper);
                cc.cmp(upper.gp, lower.gp);
                std::map<size_t, AsmJit::Label>::iterator it = bind.find(addr);
                if (it == bind.end()) bind[addr] = cc.newLabel();
                cc.je(bind[addr]);
            }
            break;
        case BC_IFICMPG:
            {
                size_t addr = bci + 1 + bcode->getInt16(bci + 1);
                var_t upper = vars.top(); vars.pop();
                var_t lower = vars.top(); vars.push(upper);
                cc.cmp(upper.gp, lower.gp);
                std::map<size_t, AsmJit::Label>::iterator it = bind.find(addr);
                if (it == bind.end()) bind[addr] = cc.newLabel();
                cc.jg(bind[addr]);
            }
            break;
        case BC_IFICMPGE:
            {
                size_t addr = bci + 1 + bcode->getInt16(bci + 1);
                var_t upper = vars.top(); vars.pop();
                var_t lower = vars.top(); vars.push(upper);
                cc.cmp(upper.gp, lower.gp);
                std::map<size_t, AsmJit::Label>::iterator it = bind.find(addr);
                if (it == bind.end()) bind[addr] = cc.newLabel();
                cc.jge(bind[addr]);
            }
            break;
        case BC_IFICMPL:
            {
                size_t addr = bci + 1 + bcode->getInt16(bci + 1);
                var_t upper = vars.top(); vars.pop();
                var_t lower = vars.top(); vars.push(upper);
                cc.cmp(upper.gp, lower.gp);
                std::map<size_t, AsmJit::Label>::iterator it = bind.find(addr);
                if (it == bind.end()) bind[addr] = cc.newLabel();
                cc.jl(bind[addr]);
            }
            break;
        case BC_IFICMPLE:
            {
                size_t addr = bci + 1 + bcode->getInt16(bci + 1);
                var_t upper = vars.top(); vars.pop();
                var_t lower = vars.top(); vars.push(upper);
                cc.cmp(upper.gp, lower.gp);
                std::map<size_t, AsmJit::Label>::iterator it = bind.find(addr);
                if (it == bind.end()) bind[addr] = cc.newLabel();
                cc.jle(bind[addr]);
            }
            break;
        case BC_CALL:
            {
                BytecodeFunction *fptr =
                                (BytecodeFunction *)m_code->functionById(bcode->getUInt16(bci + 1));
                fill_frame(&fptr->signature(), &cc, &vars);
                GPVar fptraddr = cc.newGP();
                cc.mov(fptraddr, imm((size_t)&m_fptrs[fptr->id()]));
                ECall *call = cc.call(ptr(fptraddr));
                call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder0<void>());
                cc.unuse(fptraddr);
            }
            break;
        case BC_CALLNATIVE:
            {
                Signature const *signature;
                void const *native = m_code->nativeById(bcode->getUInt16(bci + 1), &signature);
                switch ((*signature)[0].first)
                {
                case VT_INT:
                    {
                        GPVar arg(cc.newGP());
                        cc.mov(arg, imm((size_t)m_frame));
                        var_t ret(cc.newGP());
                        ECall *call = cc.call(imm((size_t)function_cast<int64_t_call>(native)));
                        call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<int64_t,size_t>());
                        call->setArgument(0, arg);
                        call->setReturn(ret.gp);
                        cc.unuse(arg);
                        vars.push(ret);
                        store_int_var(0, &cc, this, &vars);
                    }
                    break;
                case VT_STRING:
                    {
                        GPVar arg(cc.newGP());
                        cc.mov(arg, imm((size_t)m_frame));
                        var_t ret(cc.newGP());
                        ECall *call = cc.call(imm((size_t)function_cast<string_call>(native)));
                        call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<size_t,size_t>());
                        call->setArgument(0, arg);
                        call->setReturn(ret.gp);
                        cc.unuse(arg);
                        vars.push(ret);
                        store_string_var(0, &cc, this, &vars);
                    }
                    break;
                case VT_DOUBLE:
                    {
                        GPVar arg(cc.newGP());
                        cc.mov(arg, imm((size_t)m_frame));
                        var_t ret(cc.newXMM());
                        ECall *call = cc.call(imm((size_t)function_cast<double_call>(native)));
                        call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<double,size_t>());
                        call->setArgument(0, arg);
                        call->setReturn(ret.xmm);
                        cc.unuse(arg);
                        vars.push(ret);
                        store_double_var(0, &cc, this, &vars);
                    }
                    break;
                case VT_VOID:
                    {
                        GPVar arg(cc.newGP());
                        cc.mov(arg, imm((size_t)m_frame));
                        ECall *call = cc.call(imm((size_t)function_cast<void_call>(native)));
                        call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void,size_t>());
                        call->setArgument(0, arg);
                        cc.unuse(arg);
                    }
                    break;
                default: assert(0);
                }
            }
            break;
        case BC_RETURN:
            cc.jmp(exit);
            break;
        default: assert(0);
        }
        
        bci += m_code->insn_size(insn);
    }
    cc.bind(exit);
    cc.unuse(locals);
    gen_free(&cc, this, f);
    cc.ret();
    cc.endFunction();
    m_fptrs[f->id()] = function_cast<void (*)()>(cc.make());
}

void BCCompiler::push_locals(size_t id, size_t size)
{
    m_locals[id].push(new char[size]);
}

void BCCompiler::pop_locals(size_t id)
{
    m_locals[id].pop();
}

void * BCCompiler::locals(size_t id)
{
    return m_locals[id].top();
}

void BCCompiler::fill_frame(Signature const *signature,
                            Compiler *cc,
                            std::stack<var_t> *vars)
{
    size_t offset = 0;
    GPVar pframe(cc->newGP());
    cc->mov(pframe, imm((size_t)m_frame));
    for (size_t i = 1; i != signature->size(); ++i)
    {
        var_t arg = vars->top(); vars->pop();
        switch ((*signature)[i].first)
        {
        case VT_INT:
            {
                cc->mov(qword_ptr(pframe, offset), arg.gp);
                cc->unuse(arg.gp);
                offset += sizeof(int64_t);
            }
            break;
        case VT_STRING:
            {
                cc->mov(qword_ptr(pframe, offset), arg.gp);
                cc->unuse(arg.gp);
                offset += sizeof(char const *);
            }
            break;
        case VT_DOUBLE:
            {
                cc->movq(qword_ptr(pframe, offset), arg.xmm);
                cc->unuse(arg.xmm);
                offset += sizeof(double);
            }
            break;
        default: assert(0);
        }
    }
    cc->unuse(pframe);
}
