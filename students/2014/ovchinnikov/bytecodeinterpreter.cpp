#include "bytecodeinterpreter.hpp"

#include <iostream>
#include <functional>
#include <cmath>

template <typename T> int64_t compare(T left, T right) { return left > right ? 1 : left == right ? 0 : -1; }

Status *BytecodeInterpreter::execute(vector<Var *> &) try {
    #ifdef BCPRINT
    disassemble();
    return Status::Ok();
    #endif
    processCall(0);
    while (pointer() < bc()->length()) {
        Instruction instruction = next();
        switch (instruction) {
            case BC_INVALID:    throw "Invalid code";
            case BC_DLOAD:      operandStack.push_back(bc_read<double>()); break;
            case BC_ILOAD:      operandStack.push_back(bc_read<int64_t>()); break;
            case BC_SLOAD:      operandStack.push_back(bc_read<uint16_t>()); break;
            case BC_DLOAD0:     operandStack.push_back((double)0); break;
            case BC_ILOAD0:     operandStack.push_back((int64_t)0); break;
            case BC_SLOAD0:     operandStack.push_back((uint16_t)0); break;
            case BC_DLOAD1:     operandStack.push_back((double)1.0); break;
            case BC_ILOAD1:     operandStack.push_back((int64_t)1); break;
            case BC_DLOADM1:    operandStack.push_back((double) - 1.0); break;
            case BC_ILOADM1:    operandStack.push_back((int64_t) - 1); break;
            case BC_DADD:       operandStack.push_back(pop_return().d + pop_return().d); break;
            case BC_IADD:       operandStack.push_back(pop_return().i + pop_return().i); break;
            case BC_DSUB:       operandStack.push_back(pop_return().d - pop_return().d); break;
            case BC_ISUB:       operandStack.push_back(pop_return().i - pop_return().i); break;
            case BC_DMUL:       operandStack.push_back(pop_return().d * pop_return().d); break;
            case BC_IMUL:       operandStack.push_back(pop_return().i * pop_return().i); break;
            case BC_DDIV:       operandStack.push_back(pop_return().d / pop_return().d); break;
            case BC_IDIV:       operandStack.push_back(pop_return().i / pop_return().i); break;
            case BC_IMOD:       operandStack.push_back(pop_return().i % pop_return().i); break;
            case BC_IAOR:       operandStack.push_back(pop_return().i | pop_return().i); break;
            case BC_IAAND:      operandStack.push_back(pop_return().i & pop_return().i); break;
            case BC_IAXOR:      operandStack.push_back(pop_return().i ^ pop_return().i); break;
            case BC_DNEG:       operandStack.back().d = -operandStack.back().d; break;
            case BC_INEG:       operandStack.back().i = -operandStack.back().i; break;
            case BC_DPRINT:     std::cout << pop_return().d; break;
            case BC_IPRINT:     std::cout << pop_return().i; break;
            case BC_SPRINT:     std::cout << constantById(pop_return().ui16); break;
            case BC_I2D:        operandStack.back().d = operandStack.back().i; break;
            case BC_D2I :       operandStack.back().i = operandStack.back().d; break;
            case BC_SWAP:       std::iter_swap(operandStack.end() - 1, operandStack.end() - 2); break;
            case BC_POP:        operandStack.pop_back(); break;
            case BC_LOADDVAR:
            case BC_LOADIVAR:
            case BC_LOADSVAR:   operandStack.push_back(value(bc_read<uint16_t>())); break;
            case BC_STOREDVAR:
            case BC_STOREIVAR:
            case BC_STORESVAR:  value(bc_read<uint16_t>()) = pop_return(); break;
            case BC_LOADCTXDVAR:
            case BC_LOADCTXIVAR:
            case BC_LOADCTXSVAR: operandStack.push_back(value(bc_read<uint16_t>(), bc_read<uint16_t>())); break;
            case BC_STORECTXDVAR:
            case BC_STORECTXIVAR:
            case BC_STORECTXSVAR: value(bc_read<uint16_t>(), bc_read<uint16_t>()) = pop_return(); break;
            case BC_DCMP:       operandStack.push_back(compare<double>(pop_return().d, pop_return().d)); break;
            case BC_ICMP:       operandStack.push_back(compare<int64_t>(pop_return().i, pop_return().i)); break;
            case BC_JA:         pointer() += bc()->getInt16(pointer()); break;
            case BC_IFICMPE:    pointer() += pop_return().i == pop_return().i ? bc()->getInt16(pointer()) : sizeof(int16_t); break;
            case BC_IFICMPNE:   pointer() += pop_return().i != pop_return().i ? bc()->getInt16(pointer()) : sizeof(int16_t); break;
            case BC_IFICMPG:    pointer() += pop_return().i >  pop_return().i ? bc()->getInt16(pointer()) : sizeof(int16_t); break;
            case BC_IFICMPGE:   pointer() += pop_return().i >= pop_return().i ? bc()->getInt16(pointer()) : sizeof(int16_t); break;
            case BC_IFICMPL:    pointer() += pop_return().i <  pop_return().i ? bc()->getInt16(pointer()) : sizeof(int16_t); break;
            case BC_IFICMPLE:   pointer() += pop_return().i <= pop_return().i ? bc()->getInt16(pointer()) : sizeof(int16_t); break;
            case BC_CALL:       processCall(bc_read<uint16_t>()); break;
            case BC_RETURN:     processReturn(); break;
            case BC_BREAK:      break;
            case BC_DUMP:       std::cout << operandStack.back().i << std::endl; break;
            case BC_STOP:       return Status::Warning("Execution stopped");
            default: throw "Not implemented"; break;
        }
    }
    return Status::Ok();
} catch (const char *msg) {
    return Status::Error(msg);
}

void BytecodeInterpreter::processCall(uint16_t functionId) {
    auto fun = (BytecodeFunction *)functionById(functionId);
    StackFrame frame = {
        fun,
        0,
        (uint32_t)variableStack.size(),
        (uint32_t)operandStack.size() - fun->parametersNumber()
    };
    callStack.push_back(frame);
    ctxOffset[functionId].push_back(frame.v_offset);

    // move arguments to variable stack
    auto it = std::prev(operandStack.end(), frame.function->parametersNumber());
    std::move(it, operandStack.end(), std::back_inserter(variableStack));
    operandStack.erase(it, operandStack.end());

    #ifdef CALLPRINT
    if (functionId != 0) {
        std::cout << fun->name() << '(';
        copy(
            variableStack.begin() + frame.v_offset,
            variableStack.end() - 1,
            ostream_iterator<Value>(std::cout, ", ")
        );
        std::cout << variableStack.back();
        std::cout << ')' << std::endl;
    }
    #endif

    // allocate space for locals
    variableStack.resize(variableStack.size() + frame.function->localsNumber());
}

inline void BytecodeInterpreter::processReturn() {
    StackFrame frame = callStack.back();
    callStack.pop_back();
    ctxOffset[frame.function->id()].pop_back();

    // remove locals from stack
    variableStack.resize(frame.v_offset);
    // reset operand stack to previous state
    // (except for non-void function, in this case last operand, i.e. returned value is pushed onto stack back)
    VarType returnType = frame.function->returnType();
    if (returnType == VT_VOID) {
        operandStack.resize(frame.o_offset);
    } else {
        Value returnValue = operandStack.back();
        operandStack.resize(frame.o_offset);
        operandStack.push_back(returnValue);
    }
}
