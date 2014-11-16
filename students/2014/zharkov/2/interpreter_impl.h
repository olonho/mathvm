#ifndef INTERPRETER_IMPL 
#define INTERPRETER_IMPL 
#include "mathvm.h"
#include "common.h"

#include <map>
#include <string>
#include <stack>
#include <vector>
#include <exception>
#include <iostream>

namespace mathvm { 

class InterpreterCodeImpl : public Code {
    Status* execute(vector<Var*>&) {
        return Status::Ok();
    }

    void disassemble(ostream& out, FunctionFilter* filter);
};

}
#endif
