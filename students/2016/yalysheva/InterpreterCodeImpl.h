//
// Created by natalia on 11.11.16.
//
#pragma once

#include "../../../include/mathvm.h"
#include "../../../include/ast.h"

namespace mathvm {

class InterpreterCodeImpl : public Code {
public:
    virtual Status *execute(vector<Var *> &vars);
};
}
