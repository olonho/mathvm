#pragma once

#include <mathvm.h>
#include <vector>

namespace mathvm {
class InterpreterCodeImpl : public Code {
public:
  InterpreterCodeImpl(std::ostream& out) : Code(), _out(out) {}

  Status* execute(std::vector<Var*>& vars) override;

  inline BytecodeFunction* functionByName(std::string const& name) {
    return static_cast<BytecodeFunction*>(Code::functionByName(name));
  }

  inline BytecodeFunction* functionById(uint16_t id) {
    return static_cast<BytecodeFunction*>(Code::functionById(id));
  }

private:
  std::ostream& _out;
}; // InterpreterCodeImpl
} // mathvm
