#pragma once

#include <mathvm.h>
#include <vector>

namespace mathvm {
class InterpreterCodeImpl : public Code {
public:
  Status* execute(std::vector<Var*>& vars) override;
}; // InterpreterCodeImpl
} // mathvm
