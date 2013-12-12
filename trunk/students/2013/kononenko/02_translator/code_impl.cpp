#include "stdafx.h"
#include "code_impl.h"
#include "interpreter.h"

namespace mathvm
{

    
code_impl::code_impl()
{
    zero_string_id_ = makeStringConstant("");
}

code_impl::~code_impl()
{
}

Status* code_impl::execute(vector<Var*>& vars)
{
    interpreter ir;
    
    try
    {
        ir.interpret(this);
        return new Status();
    }
    catch (error const &e)
    {
        return new Status(e.what());
    }
}

function_id_t code_impl::add_function()
{
    funcs_.push_back(function_impl());
    return funcs_.size() - 1;
}

function_impl *code_impl::get_function_dst(function_id_t id)
{
    return &funcs_.at(id);
}


} // namespace mathvm