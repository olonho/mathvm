#pragma once

#include "mathvm.h"
#include "defs.h"
#include "function.h"
#include "interpreted.h"

namespace mathvm
{

struct function_impl
    : function_t
{
    function_impl(context_id_t id)
        : context_id_(id)
    {
    }
    
    Bytecode const *bytecode() OVERRIDE { return &bytecode_; } 
    bool has_local_context(size_t pos) OVERRIDE { return true; }
    context_id_t local_context(size_t pos) OVERRIDE { return context_id_; }

    void set_context(context_id_t id)
    {
    }
    Bytecode *bytecode_dst()
    {
        return &bytecode_;
    }

private:
    Bytecode bytecode_;
    context_id_t context_id_;
};

struct code_impl
    : Code
    , private interpreted
{
    
    code_impl();
    ~code_impl();
    
    Status* execute(vector<Var*>& vars) OVERRIDE;

public:
    function_id_t add_function();
    function_impl *get_function_dst(function_id_t id);

private:
    //interpreted

    function_id_t get_top_function() OVERRIDE { return 0; }
    function_id_t num_functions() OVERRIDE  { return funcs_.size(); }
    function_t *get_function(function_id_t id) OVERRIDE { return &funcs_.at(id); }
    string const& get_string_const(int16_t id) const OVERRIDE { return constantById(id); }

private:
    vector<function_impl> funcs_;

    int16_t zero_string_id_;
};



} //namespace mathvm