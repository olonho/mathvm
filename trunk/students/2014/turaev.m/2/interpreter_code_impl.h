#ifndef INTERPRETER_CODE_IMPL_H
#define INTERPRETER_CODE_IMPL_H

#include "mathvm.h"

namespace mathvm {
class InterpreterCodeImpl: public Code {
    virtual Status* execute(vector<Var*>& vars) {
        return 0;
    }   
};
}

#endif