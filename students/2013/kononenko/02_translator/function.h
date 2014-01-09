#pragma once

namespace mathvm
{

struct function_t
{
    virtual ~function_t() {};

    virtual Bytecode const *bytecode() = 0;
    virtual bool has_local_context(size_t pos) = 0;
    virtual context_id_t local_context(size_t pos) = 0;
};

} // namespace mathvm