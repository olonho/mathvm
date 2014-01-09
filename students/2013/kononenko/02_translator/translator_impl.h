#pragma once

#include "ast.h"
#include "code_impl.h"

namespace mathvm
{

struct translator_impl
    : Translator
    , AstVisitor
{
    translator_impl();
    
    Status* translate(const string& program, Code **code);

#define VISITOR_FUNCTION(type, name) void visit##type(type * node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
    struct context_t
    {
        context_t(context_id_t id)
            : id(id)
        { }

        context_id_t id;

        typedef map<string, var_id_t> vars_t;
        vars_t vars;

        typedef map<string, function_id_t> functions_t;
        functions_t functions;
    };
private:
    void init();
    void prepare(AstFunction *top);

    
    void op_arithm (TokenKind op, VarType type1, VarType type2);
    void op_comp   (TokenKind op, VarType type1, VarType type2);
    void op_andor  (TokenKind op, VarType type1, VarType type2);
    void op_bitwise(TokenKind op, VarType type1, VarType type2);

    Instruction make_instruction(TokenKind op, VarType type1, VarType type2);
    std::pair<context_id_t, var_id_t> get_var_ids(AstVar const *var, bool store, bool *out_is_local);

    void prepare_scope(Scope *scope, context_t &dst_context, function_id_t fn_id);
    
    void load_tos_var(AstVar const *var);
    void store_tos_var(AstVar const *var);
    Instruction get_var_insn(bool store, AstVar const *var, bool is_local);
    void process_var(bool store, AstVar const *var);

    function_id_t find_function(string const &name) const;

    context_id_t new_context_id() const;

private:
    Bytecode *bytecode()
    {
        return dst_code_->get_function_dst(dst_func_id_)->bytecode_dst();
    }


    context_t &current_context() 
    {
        return fn_contexts_.at(dst_func_id_);
    }

    const context_t &current_context() const
    {
        return fn_contexts_.at(dst_func_id_);
    }

private:
    VarType tos_type_;
    code_impl *dst_code_;
    function_id_t dst_func_id_;
    Scope *current_scope_;
    
    // used only to pass signature from function node to inner block node
    Signature const *signature_;

    bool return_;
    
    //typedef map<Scope const*, context_t> contexts_t;
    //contexts_t contexts_;

    map<function_id_t, context_t> fn_contexts_;
    map<Scope const*, function_id_t> scope2fn_;

};

} // namespace mathvmo