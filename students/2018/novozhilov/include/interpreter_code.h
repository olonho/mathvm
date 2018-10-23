//
// Created by jetbrains on 18.10.18.
//

#ifndef VIRTUAL_MACHINES_INTERPRETER_CODE_H
#define VIRTUAL_MACHINES_INTERPRETER_CODE_H

#include <mathvm.h>

namespace mathvm {
    class InterpreterCodeImpl : public Code {
    public:
        Status *execute(vector<Var *> &vars) override;
    };
}

#endif //VIRTUAL_MACHINES_INTERPRETER_CODE_H
