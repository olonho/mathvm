#ifndef FUNCTIONID_H
#define FUNCTIONID_H

#include "mathvm.h"

struct FunctionId {
    mathvm::AstFunction* function;
    uint16_t id;
    FunctionId (mathvm::AstFunction* f, uint16_t i): function(f), id(i) {}
    FunctionId(): function(NULL), id(0) {}
};

#endif
