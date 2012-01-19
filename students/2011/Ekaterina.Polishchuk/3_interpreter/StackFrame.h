#ifndef STACKFRAME_H
#define STACKFRAME_H

#include "StackVariable.h"
#include <vector>

struct StackFrame {
    std::vector<StackVariable> vars;
    uint32_t ip;
    StackFrame * prevFrame;
    uint16_t functionId;
    StackFrame(uint16_t variablesNum, uint16_t functionId) :  ip(0), prevFrame(0), functionId(functionId) {
        vars.resize(variablesNum);
    }
};
#endif
