#ifndef BYTECODEINTERPRETER_HPP
#define BYTECODEINTERPRETER_HPP

#include <vector>
#include <map>

using std::vector;
using std::map;

#include "mathvm.h"

using namespace mathvm;


class BytecodeInterpreter : public Code {

    typedef union _v {
        double d;
        int64_t i;
        uint16_t ui16;
        _v() {}
        _v(double d) : d(d) {}
        _v(int64_t i) : i(i) {}
        _v(uint16_t ui): ui16(ui) {}
    } Value;

    struct StackFrame {
        StackFrame *previous;
        uint32_t v_offset;
        uint32_t o_offset;
        uint32_t return_pointer;
        BytecodeFunction *function;
    };

public:
    BytecodeInterpreter() {}
    ~BytecodeInterpreter() {}
    virtual Status *execute(vector<Var *> &);

private:
    void processCall(uint16_t functionId);
    void processReturn();

    Value pop_return() {
        Value result = operandStack.back();
        operandStack.pop_back();
        return result;
    }

    template<class T> T bc_read() {
        T v = bc->getTyped<T>(pointer);
        pointer += sizeof(T);
        return v;
    }

    vector<Value> operandStack;
    vector<Value> variableStack;
    map<uint16_t, vector<uint32_t>> ctxOffset;
    StackFrame *frame;
    Bytecode *bc;
    uint32_t pointer;
};

#endif // BYTECODEINTERPRETER_HPP
