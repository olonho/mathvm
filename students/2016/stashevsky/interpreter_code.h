#pragma once

#include "mathvm.h"

namespace mathvm {

class InterpreterCodeImpl : public Code {
public:
    InterpreterCodeImpl() {}
    virtual ~InterpreterCodeImpl() {}

    Status* execute(vector<Var*>& vars);
private:
};

}
