#ifndef INTERPRETER_CODE_HPP
#define INTERPRETER_CODE_HPP

#include "mathvm.h"

#include <map>
#include <string>
#include <utility>

#include <stdint.h>

namespace mathvm {

class InterpreterFunction : public BytecodeFunction {
  uint16_t deepness_; // how deep is function in ast (0 for top)

public:
  InterpreterFunction(AstFunction* function, uint16_t deepness) 
    : BytecodeFunction(function),
      deepness_(deepness) {}

  virtual ~InterpreterFunction() {}

  uint16_t deepness() const { return deepness_; }
};

class InterpreterCodeImpl : public Code {
  public:
    InterpreterCodeImpl() {}
    virtual ~InterpreterCodeImpl() {}

    virtual Status* execute(vector<Var*>& vars) {
      return Status::Error("Not implemented InterpreterCodeImpl::execute");
    }

    InterpreterFunction* functionByName(const string& name) {
      return dynamic_cast<InterpreterFunction*>(Code::functionByName(name));
    }

    InterpreterFunction* functionById(uint16_t id) {
      return dynamic_cast<InterpreterFunction*>(Code::functionById(id));
    }
};

}

#endif 