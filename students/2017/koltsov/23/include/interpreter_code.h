#pragma once

#include "mathvm.h"

namespace mathvm {

class InterpreterCodeImpl : public Code {
    Status* execute(vector<Var*>& vars);
    void dissasemble(ostream& out, FunctionFilter* filter);
};

}
