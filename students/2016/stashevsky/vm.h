#pragma once

#include "mathvm.h"

#include <iostream>
#include <vector>
#include <stack>

namespace mathvm {

namespace details {

union StackUnit {
    StackUnit() : id(0) {}
    StackUnit(double v) : double_value(v) {}
    StackUnit(int64_t v) : int_value(v) {}
    StackUnit(uint16_t v) : id(v) {}

    uint16_t id;
    double double_value;
    int64_t int_value;
};

struct Context {
    Context(uint32_t locals_number) {
        locals.resize(locals_number);
    }

    std::vector<StackUnit> locals;
};


}

struct vm
{
    vm(Code &code, ostream& output);
    ~vm() {}

    void run();

private:
    uint32_t STACK_SIZE = 1024 * 1024;

    Code &code_;
    ostream& output_;

    uint32_t ip_ = 0;

    std::vector<details::StackUnit> stack_;
    std::stack<uint32_t> call_stack_;
    std::stack<BytecodeFunction*> function_stack_;
    std::stack<uint64_t> stack_frames_;

    std::stack<details::Context> locals_;
    std::vector<std::stack<details::Context*>> contexts_;

    void repl();
    Bytecode& bytecode();
};

}

