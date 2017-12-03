#pragma once

#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

namespace mathvm {

class InterpreterCodeImpl : public Code {
    virtual Status* execute(vector<Var*>& vars) {
        // TODO
        assert(false);
        return nullptr;
    };
};

}
