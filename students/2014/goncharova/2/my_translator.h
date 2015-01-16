#pragma once

#include "parser.h"
#include "common.h"
#include "my_interpreter.h"

using namespace mathvm;

class BytecodeImpl: public Code {
    //dummy impl of Code that stores translated bytecode
    virtual Status* execute(vector<Var*>& vars);
};