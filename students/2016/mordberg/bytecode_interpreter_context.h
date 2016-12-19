#pragma once

#include "mathvm.h"
#include "ast.h"

#include <vector>

namespace mathvm {

namespace interpreter {

struct BytecodeVar {
    union {
        int64_t i;
        double d;
        const char *s;
    } _value;

    BytecodeVar() { _value.i = 0; }

    explicit BytecodeVar(int64_t value) {
        _value.i = value;
    }

    explicit BytecodeVar(double value) {
        _value.d = value;
    }

    explicit BytecodeVar(const char *value) {
        _value.s = value;
    }
};

template <class T>
BytecodeVar pack(T value) {
    return BytecodeVar(value);
}

class BytecodeInterpreterContext {
public:

    BytecodeInterpreterContext(uint32_t address, uint32_t stackframe, BytecodeFunction* function, uint32_t locals_count);

    Bytecode *get_bytecode() const;

    uint32_t get_call_address() const;

    uint32_t get_stackframe() const;

    VarType get_return_type() const;

    uint16_t id() const;

    BytecodeVar get_local_value(uint16_t id) const;

    void set_local_value(uint16_t id, BytecodeVar value);

private:
    uint32_t _call_address;
    uint32_t _stackframe;
    BytecodeFunction *_bf;
    std::vector<BytecodeVar> _local_vars;
};

} // namespace interpreter

} // namespace mathvm