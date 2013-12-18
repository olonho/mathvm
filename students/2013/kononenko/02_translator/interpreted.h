#pragma once

#include "function.h"

namespace mathvm
{

struct interpreted
{
    virtual ~interpreted() {};

    virtual function_id_t get_top_function() = 0;
    virtual function_id_t num_functions() = 0;
    virtual function *get_function(function_id_t id) = 0;
    virtual string const& get_string_const(int16_t id) const = 0;
};

} // namespace mathvm