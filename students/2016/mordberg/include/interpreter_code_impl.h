#pragma once

#include "mathvm.h"

namespace mathvm {

class InterpreterCodeImpl : public Code {
public:
    InterpreterCodeImpl() {}

    Status* execute(vector<Var*>& vars) {
        return Status::Ok();
    }
};

}