#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#include "mathvm.h"

struct Interpreter: mathvm::Code {
    mathvm::Status* execute(std::vector<mathvm::Var*>& vars) {
        disassemble();
        return 0;
    }
};

#endif
