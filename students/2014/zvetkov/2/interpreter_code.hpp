#ifndef INTERPRETER_CODE_HPP
#define INTERPRETER_CODE_HPP

#include "mathvm.h"

namespace mathvm {

class InterpreterCodeImpl : public Code {
  public:
    InterpreterCodeImpl();
    virtual ~InterpreterCodeImpl();

    virtual Status* execute(vector<Var*>& vars);

    BytecodeFunction* functionByName(const string& name);
    BytecodeFunction* functionById(uint16_t id);
};

}

#endif 