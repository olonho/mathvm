#ifndef CONTEXT_H
#define CONTEXT_H

#include <cstddef>
#include <memory>
#include "mathvm.h"

namespace mathvm
{

class context
{
public:
    context(BytecodeFunction* top)
        : id_(0)
        , function_(top)
        , parent_(nullptr)
    {}

    virtual ~context() {}

    BytecodeFunction* function()
    {
        return function_;
    }

    Bytecode* bc()
    {
        return function_->bytecode();
    }

    uint16_t id() const
    {
        return id_;
    }

    context* parent()
    {
        return parent_;
    }

    context* up(BytecodeFunction* func = nullptr)
    {
        if (func == nullptr)
            func = function();

        return new context(id() + 1, func, this);
    }

    context* down()
    {
        std::unique_ptr<context> bye_bye(this);
        return parent();
    }

protected:
    context(uint16_t id, BytecodeFunction* func, context* parent)
        : id_(id)
        , function_(func)
        , parent_(parent)
    {}

protected:
    uint16_t id_;
    BytecodeFunction* function_;
    context* parent_;
};

} // mathvm namespace

#endif // CONTEXT_H
