#include "include/interpreter_context.h"

using namespace mathvm;

StackValue InterpreterContext::getVarById(uint16_t index) {
    if (index >= _variables.size()) {
        throw std::runtime_error("index out of bounds");
    }
    return _variables[index];
}

void InterpreterContext::setVarById(StackValue var, uint16_t index) {
    _variables[index] = std::move(var);
}

InterpreterContext::InterpreterContext(BytecodeFunction *function) : _variables(function->localsNumber()) {}
