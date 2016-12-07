#include "bytecode_interpreter_context.h"


using namespace mathvm;
using namespace mathvm::interpreter;


Bytecode *BytecodeInterpreterContext::get_bytecode() const {
    return _bf->bytecode();
}

uint32_t BytecodeInterpreterContext::get_call_address() const {
    return _call_address;
}

uint32_t BytecodeInterpreterContext::get_stackframe() const {
    return _stackframe;
}

BytecodeVar BytecodeInterpreterContext::get_local_value(uint16_t id) const {
    return _local_vars[id];
}

void BytecodeInterpreterContext::set_local_value(uint16_t id, BytecodeVar value) {
    _local_vars[id] = value;
}

BytecodeInterpreterContext::BytecodeInterpreterContext(uint32_t address, uint32_t stackframe,
                                                       mathvm::BytecodeFunction *function, uint32_t locals_count)
        : _call_address(address)
        , _stackframe(stackframe)
        , _bf(function)
        , _local_vars(locals_count) {}

uint16_t BytecodeInterpreterContext::id() const {
    return _bf->id();
}

VarType BytecodeInterpreterContext::get_return_type() const {
    return _bf->returnType();
}
