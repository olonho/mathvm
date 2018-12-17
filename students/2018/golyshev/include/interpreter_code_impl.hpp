#pragma once

#include <mathvm.h>
#include "context.hpp"

namespace mathvm {
    class InterpreterCodeImpl : public Code {
    public:
        using NativeFunction = void(*)(NativeVal*, NativeVal*);
        mathvm::Status* execute(std::vector<mathvm::Var*>& vars) override;
    };

}