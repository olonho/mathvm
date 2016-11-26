#pragma once

#include "mathvm.h"

#include <iostream>
#include <vector>
#include <stack>
#include <ast.h>

#include "asmjit/asmjit.h"

namespace mathvm {
namespace details {

union StackUnit {
    StackUnit() : int_value(0) {}
    StackUnit(double v) : double_value(v) {}
    StackUnit(int64_t v) : int_value(v) {}

    double double_value;
    int64_t int_value;
};

struct Context {
    Context(uint32_t locals_number) {
        locals.resize(locals_number);
    }

    vector<StackUnit> locals;
};

}

struct vm
{
    vm(Code &code, ostream& output);
    ~vm() {}

    int run();

private:
    typedef void* (*native_handler)(void const*);

    uint32_t STACK_SIZE = 1024 * 1024;

    Code &code_;
    ostream& output_;

    vector<details::StackUnit> stack_;
    stack<uint32_t> call_stack_;
    stack<BytecodeFunction*> function_stack_;
    stack<uint64_t> stack_frames_;

    stack<details::Context> locals_;
    vector<stack<details::Context*>> contexts_;

    asmjit::JitRuntime runtime_;

    int repl();

    Bytecode& bytecode();
};

}

