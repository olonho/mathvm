#pragma once

#include <ast.h>
#include "mathvm.h"
#include "vm.h"

namespace mathvm {

class InterpreterCodeImpl : public Code {
public:
    InterpreterCodeImpl() {}
    virtual ~InterpreterCodeImpl() {}

    virtual Status* execute(vector<Var*>& vars);
private:
};

}
