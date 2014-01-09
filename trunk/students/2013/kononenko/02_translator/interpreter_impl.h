#pragma once

#include "interpreted.h"

namespace mathvm
{

namespace interpreter_detail
{

template<typename T>
inline T read_from_bc(interpreted const * /*code*/, Bytecode const *bc, size_t &pos)
{
    const T res = bc->getTyped<T>(pos);
    pos += sizeof(T);
    return res;
}

template<>
inline const char* read_from_bc<const char*>(interpreted const *code, Bytecode const *bc, size_t &pos)
{
    const int16_t id = read_from_bc<int16_t>(code, bc, pos);
    return code->get_string_const(id).c_str();
}


}

struct interpreter
{
    void interpret(interpreted *code);

private:
    void process_func();

private:
    context_id_t context_for_pos(size_t pos) const;
    void process_insn(Instruction insn);

    template<typename T> void process_load();
    template<typename T> void process_load_val(T val);
    template<typename T> void process_binary(Instruction insn);
    template<typename T> void process_unary(Instruction insn);

    template<typename T> T process_binary_impl(Instruction insn, T val1, T val2);
    i_t process_binary_impl(Instruction insn, i_t val_1, i_t val_2);
    d_t process_binary_impl(Instruction insn, d_t val_1, d_t val_2);

    template<typename T> void process_print();
    template<typename T> void process_load_local();
    template<typename T> void process_load_ctx();
    template<typename T> void process_load_var(context_id_t context_id, var_id_t var_id);
    template<typename T> void process_store_local();
    template<typename T> void process_store_ctx();
    template<typename T> void process_store_var(context_id_t context_id, var_id_t var_id);

    void process_jump(Instruction insn);
    
    void process_call();

private:
    template<typename T> T read()
    {
        return interpreter_detail::read_from_bc<T>(code_, func_->bytecode(), pos_);
    }

private:
    interpreted *code_;
    function_t *func_;

    typedef map<std::pair<context_id_t, var_id_t>, Var> vars_t;

    struct context_t
    {
        map<var_id_t, Var> vars;
    };

    typedef stack<context_t> contexts_stack_t;
    typedef map<function_id_t, contexts_stack_t> fn_contexts_stacks_t;

    fn_contexts_stacks_t fn_contexts_stacks_;

    typedef std::stack<Var> stack_t;
    stack_t stack_;

    size_t pos_;
    int32_t jump_offset_;
    context_id_t context_id_;

    bool return_;
};

} // namespace mathvm