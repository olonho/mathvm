#ifndef STACKVARIABLE_H
#define STACKVARIABLE_H

#include "mathvm.h"

struct StackVariable {
    union {
        double d;
        int64_t i;
        char const * s;
    };
};
#endif
