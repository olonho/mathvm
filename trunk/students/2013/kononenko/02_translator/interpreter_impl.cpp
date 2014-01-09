#include "stdafx.h"
#include "interpreter_impl.h"

namespace mathvm
{

namespace
{
struct unsupported_insn 
    : error
{
    unsupported_insn(Instruction insn)
        : error(string("Unsupported instruction: ") + bytecodeName(insn))
    { }
};

}


void interpreter::interpret(interpreted *code)
{
#ifdef PRINT_DUMP
    std::ofstream s("dump.txt");
    for (size_t i = 0; i < code->num_functions(); ++i)
    {
        s << "Function " << i << endl << endl;

        code->get_function(i)->bytecode()->dump(s);
        
        s << endl;
    }
    s.close();
#endif

    code_ = code;
    func_ = code_->get_function(code_->get_top_function());
    process_func();
}

void interpreter::process_func()
{
    context_id_ = func_->local_context();
    fn_contexts_stacks_[context_id_].push(context_t());
    
    for (size_t pos = 0; pos < func_->bytecode()->length();)
    {
        jump_offset_ = 0;
        size_t length;
        const Instruction insn = func_->bytecode()->getInsn(pos);
        const char* name = bytecodeName(insn, &length);
        (void)name;


        pos_ = pos + 1;
        
        return_ = false;
        process_insn(insn);

        if (return_)
        {
            return_ = false;
            break;
        }
        
        if (jump_offset_ != 0)
        {
            pos = pos_ + jump_offset_;
            continue;
        }

        pos += length;
    }

    fn_contexts_stacks_.at(context_id_).pop();
    if (fn_contexts_stacks_.at(context_id_).empty())
        fn_contexts_stacks_.erase(context_id_);
}


void interpreter::process_insn(Instruction insn)
{
    switch(insn)
    {
    case BC_DLOAD: process_load<d_t>(); break;
    case BC_ILOAD: process_load<i_t>(); break;
    case BC_SLOAD: process_load<s_t>(); break;
    case BC_DLOAD0: process_load_val<d_t>(0); break;
    case BC_ILOAD0: process_load_val<i_t>(0); break;
    case BC_DLOAD1: process_load_val<d_t>(1); break;
    case BC_ILOAD1: process_load_val<i_t>(1); break;
    case BC_DLOADM1: process_load_val<d_t>(-1); break;
    case BC_ILOADM1: process_load_val<i_t>(-1); break;

    case BC_DADD: process_binary<d_t>(insn); break;
    case BC_IADD: process_binary<i_t>(insn); break;
    case BC_DSUB: process_binary<d_t>(insn); break;
    case BC_ISUB: process_binary<i_t>(insn); break;
    case BC_DMUL: process_binary<d_t>(insn); break;
    case BC_IMUL: process_binary<i_t>(insn); break;
    case BC_DDIV: process_binary<d_t>(insn); break;
    case BC_IDIV: process_binary<i_t>(insn); break;
    case BC_IMOD: process_binary<i_t>(insn); break;

    case BC_DNEG: process_unary<d_t>(insn); break;
    case BC_INEG: process_unary<i_t>(insn); break;

    case BC_IAOR: process_binary<i_t>(insn); break;
    case BC_IAAND: process_binary<i_t>(insn); break;
    case BC_IAXOR: process_binary<i_t>(insn); break;

    case BC_IPRINT: process_print<i_t>(); break;
    case BC_DPRINT: process_print<d_t>(); break;
    case BC_SPRINT: process_print<s_t>(); break;

    case BC_POP: stack_.pop(); break;

    case BC_LOADDVAR: process_load_local<d_t>(); break;
    case BC_LOADIVAR: process_load_local<i_t>(); break;
    case BC_LOADSVAR: process_load_local<s_t>(); break;
    case BC_STOREDVAR: process_store_local<d_t>(); break;
    case BC_STOREIVAR: process_store_local<i_t>(); break;
    case BC_STORESVAR: process_store_local<s_t>(); break;

    case BC_LOADCTXDVAR: process_load_ctx<d_t>(); break;
    case BC_LOADCTXIVAR: process_load_ctx<i_t>(); break;
    case BC_LOADCTXSVAR: process_load_ctx<s_t>(); break;
    case BC_STORECTXDVAR: process_store_ctx<d_t>(); break;
    case BC_STORECTXIVAR: process_store_ctx<i_t>(); break;
    case BC_STORECTXSVAR: process_store_ctx<s_t>(); break;

    case BC_JA      : 
    case BC_IFICMPNE: 
    case BC_IFICMPE : 
    case BC_IFICMPG : 
    case BC_IFICMPGE: 
    case BC_IFICMPL : 
    case BC_IFICMPLE: process_jump(insn); break;
    
    case BC_CALL: process_call(); break;
    case BC_RETURN: return_ = true; break;

    default: throw unsupported_insn(insn);
    }
}

template<typename T>
void interpreter::process_load()
{
    const T val = read<T>();
    stack_.push(create_val(val));
}


template<typename T>
void interpreter::process_load_val(T val)
{
    stack_.push(create_val(val));
}


template<typename T>
void interpreter::process_binary(Instruction insn)
{
    const T val2 = get_val<T>(stack_.top());
    stack_.pop();
    const T val1 = get_val<T>(stack_.top());
    stack_.pop();

    const T res = process_binary_impl(insn, val1, val2);
    stack_.push(create_val(res));
}

i_t interpreter::process_binary_impl(Instruction insn, i_t val1, i_t val2)
{
    i_t res;

    switch(insn)
    {
    case BC_IADD: res = val1 + val2; break;
    case BC_ISUB: res = val1 - val2; break;
    case BC_IMUL: res = val1 * val2; break;
    case BC_IDIV: res = val1 / val2; break;

    case BC_IMOD: res = val1 % val2; break;

    case BC_IAOR : res = val1 | val2; break;
    case BC_IAAND: res = val1 & val2; break;
    case BC_IAXOR: res = val1 ^ val2; break;

    default: throw unsupported_insn(insn);
    }

    return res;
}

d_t interpreter::process_binary_impl(Instruction insn, d_t val1, d_t val2)
{
    d_t res;

    switch(insn)
    {
    case BC_DADD: res = val1 + val2; break;
    case BC_DSUB: res = val1 - val2; break;
    case BC_DMUL: res = val1 * val2; break;
    case BC_DDIV: res = val1 / val2; break;

    default: throw unsupported_insn(insn);
    }

    return res;
}

template<typename T>
void interpreter::process_unary(Instruction insn)
{
    const T val = get_val<T>(stack_.top());
    stack_.pop();

    T res;

    switch(insn)
    {
    case BC_DNEG:
    case BC_INEG: res = -val; break;
    default: throw unsupported_insn(insn);
    }

    stack_.push(create_val(res));
}

template<typename T>
void interpreter::process_print()
{
    cout << get_val<T>(stack_.top());
    stack_.pop();
}



template<typename T>
void mathvm::interpreter::process_load_local()
{
    const var_id_t var_id = read<var_id_t>();
    process_load_var<T>(context_id_, var_id);
}

template<typename T>
void interpreter::process_load_ctx()
{
    const context_id_t context_id = read<context_id_t>();
    const var_id_t var_id = read<var_id_t>();
    process_load_var<T>(context_id, var_id);
}

template<typename T>
void interpreter::process_load_var(context_id_t context_id, var_id_t var_id)
{
    const contexts_stack_t &cs = fn_contexts_stacks_.at(context_id);
    assert(!cs.empty());
    const context_t &context = cs.top();

    // Var not initialized
    if (context.vars.count(var_id) == 0)
    {
        stack_.push(Var(var_type_t<T>::type(), ""));
    }
    else
    {
        stack_.push(context.vars.at(var_id));
    }
}

template<typename T>
void mathvm::interpreter::process_store_local()
{
    const var_id_t var_id = read<var_id_t>();
    process_store_var<T>(context_id_, var_id);
}

template<typename T>
void interpreter::process_store_ctx()
{
    const context_id_t context_id = read<context_id_t>();
    const var_id_t var_id = read<var_id_t>();
    process_store_var<T>(context_id, var_id);
}

template<typename T>
void interpreter::process_store_var(context_id_t context_id, var_id_t var_id)
{
    contexts_stack_t &cs = fn_contexts_stacks_.at(context_id);
    assert(!cs.empty());
    context_t &context = cs.top();

    if (context.vars.count(var_id) == 0)
        context.vars.insert(make_pair(var_id, stack_.top()));
    else
        context.vars.at(var_id) = stack_.top();
    
    stack_.pop();
}

void interpreter::process_jump(Instruction insn)
{
    bool jump = insn == BC_JA;

    if (insn != BC_JA)    
    {
        Var var2 = stack_.top();
        stack_.pop();
        Var var1 = stack_.top();
        stack_.pop();

        if (var1.type() != VT_INT || var2.type() != VT_INT)
            throw error("Ints required for jump comparison");

        i_t val1 = var1.getIntValue();
        i_t val2 = var2.getIntValue();

        switch (insn)
        {
        case BC_IFICMPNE: jump = val1 != val2; break;
        case BC_IFICMPE : jump = val1 == val2; break;
        case BC_IFICMPG : jump = val1 >  val2; break;
        case BC_IFICMPGE: jump = val1 >= val2; break;
        case BC_IFICMPL : jump = val1 <  val2; break;
        case BC_IFICMPLE: jump = val1 <= val2; break;
        default: assert(false);
        }
    }

    if (jump)
    {
        jump_offset_ = read<int16_t>() - 2;
    }
}

void interpreter::process_call()
{
    function_t *old_func = func_; 
    const function_id_t id = read<function_id_t>();
    
    func_ = code_->get_function(id);
    context_id_t old_context_id = context_id_;   
    process_func();
    context_id_ = old_context_id;

    func_ = old_func;
}

} // namespace mathvm
