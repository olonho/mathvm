#ifndef _INTERPRETER_STACK_H
#define _INTERPRETER_STACK_H

#include <stdint.h>

typedef int64_t Int;
typedef double  Double;
typedef const char* String;

union StackValue {
    StackValue() = default;
    StackValue(Double d) : d(d) {}
    StackValue(Int    i) : i(i) {}
    StackValue(String s) : s(s) {}

    operator Double() const { return d; }
    operator Int   () const { return i; }
    operator String() const { return s; }

    Double d;
    Int    i;
    String s;
};

struct StackFrame {
    uint16_t functionId;
    uint32_t prevContextStart;
    uint32_t bci;
};

#endif // _INTERPRETER_STACK_H
