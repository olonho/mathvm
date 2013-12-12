#pragma once

#include "mathvm.h"
#include "defs.h"
#include "function.h"
#include "interpreted.h"

namespace mathvm
{

struct function_impl
    : function
{
    function_impl()
    {
        for(size_t i = 0; i < 8; ++i)
        {
            aaa[i] = 'a';
            bbb[i] = 'b';
        }
    }
    
    Bytecode const *bytecode() OVERRIDE { return &bytecode_; } 
    bool has_local_context(size_t pos) OVERRIDE { return context_ids_.count(pos) != 0; }
    context_id_t local_context(size_t pos) OVERRIDE { return context_ids_.at(pos); }

    void set_context(context_id_t id)
    {
        const size_t pos = bytecode_.length();
        context_ids_[pos] = id;
//         const bool inserted = context_ids_.insert(make_pair(pos, id)).second;
//         assert(inserted);
    }
    Bytecode *bytecode_dst()
    {
        return &bytecode_;
    }

private:
    Bytecode bytecode_;
    typedef map<size_t, context_id_t> context_ids_t;
    
    char aaa[8];
    context_ids_t context_ids_;
    char bbb[8];
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
    function *get_function(function_id_t id) OVERRIDE { return &funcs_.at(id); }
    string const& get_string_const(int16_t id) OVERRIDE { return constantById(id); }

private:
    vector<function_impl> funcs_;

    int16_t zero_string_id_;
};



} //namespace mathvm