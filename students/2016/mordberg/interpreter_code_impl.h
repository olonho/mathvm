#pragma once

#include "mathvm.h"
#include "bytecode_interpreter_context.h"

#include <stack>
#include <ostream>

namespace mathvm {

using interpreter::BytecodeVar;
using interpreter::BytecodeInterpreterContext;

namespace {

class BytecodeReader {
public:
    BytecodeReader(Bytecode *_bytecode, uint32_t index = 0) : _bytecode(_bytecode), _index(index) {}

    bool empty() const {
        return _index >= _bytecode->length();
    }

    uint32_t index() const {
        return _index;
    }

    void jump(int32_t offset) {
        _index += offset;
    }

#define READ_FUNCTION(type, method, offset)         \
        type read_##type() {                        \
            auto res = _bytecode->method(_index);   \
            _index += offset;                       \
            return res;                             \
        }

        READ_FUNCTION(uint16_t, getUInt16, sizeof(uint16_t))
        READ_FUNCTION(int16_t, getInt16, sizeof(int16_t))
        READ_FUNCTION(Instruction, getInsn, 1)
        READ_FUNCTION(int64_t, getInt64, sizeof(int64_t))
        READ_FUNCTION(double, getDouble, sizeof(double))

#undef READ_FUNCTION

    private:
        Bytecode* _bytecode;
        uint32_t _index;
    };

} // anonymous namespace

class InterpreterCodeImpl : public Code {
public:

    InterpreterCodeImpl(ostream &_out);

    ~InterpreterCodeImpl();

    Status* execute(vector<Var*>& vars);

private:

    std::vector<BytecodeVar>              _stack;
    std::ostream&                        _out;
    Status*                              _status;

    std::stack<BytecodeInterpreterContext> _contexts;
    std::map<uint32_t , std::stack<BytecodeInterpreterContext*> > _loaded_contexts;

    BytecodeVar pop();
    template <class T>
    void push(T value) {
        _stack.push_back(interpreter::pack(value));
    }

    void push(BytecodeVar value);

    void load_var_ctx(BytecodeReader& reader);
    void load_var_local(BytecodeReader& reader);
    void load_var_cached(BytecodeReader& reader, uint16_t id);

    void store_var_ctx(BytecodeReader& reader);
    void store_var_local(BytecodeReader& reader);
    void store_var_cached(uint16_t id);
    void call_native(const Signature* signature, void* func);

};

} // namespace mathvm