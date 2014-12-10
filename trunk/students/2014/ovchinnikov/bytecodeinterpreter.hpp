#ifndef BYTECODEINTERPRETER_HPP
#define BYTECODEINTERPRETER_HPP

#include <vector>
#include <map>

using std::vector;
using std::map;

#include "mathvm.h"
#include "stuff.hpp"

using namespace mathvm;

class BytecodeInterpreter : public Code {

    struct StackFrame {
        BytecodeFunction *function;
        uint32_t pointer;
        uint32_t v_offset;
        uint32_t o_offset;
    };

public:
    BytecodeInterpreter() {}
    virtual ~BytecodeInterpreter() {}
    virtual Status *execute(vector<Var *> &);

private:
    void processCall(uint16_t functionId);
    void processNativeCall(uint16_t functionId);
    void processReturn();

    Value pop_return() {
        Value result = operandStack.back();
        operandStack.pop_back();
        return result;
    }

    template<class T> T bc_read() {
        T v = bc()->getTyped<T>(pointer());
        pointer() += sizeof(T);
        return v;
    }

    Bytecode *bc() { return callStack.back().function->bytecode(); }
    uint32_t & pointer() { return callStack.back().pointer; }
    Instruction next() { return (Instruction)bc_read<uint8_t>(); }
    Value & value(uint32_t varID) { return variableStack[callStack.back().v_offset + varID]; }
    Value & value(uint32_t ctxID, uint32_t varID) { return variableStack[ctxOffset[ctxID].back() + varID]; }

    vector<Value> operandStack;
    vector<Value> variableStack;
    map<uint16_t, vector<uint32_t>> ctxOffset;
    vector<StackFrame> callStack;

};

#endif // BYTECODEINTERPRETER_HPP
