#include "bytecodeinterpreter.hpp"

#include <iostream>
#include <functional>
#include <cmath>

using std::cout;
using std::showpoint;
using std::endl;

template <typename T> int64_t compare(T left, T right) { return left > right ? 1 : left == right ? 0 : -1; }

Status *BytecodeInterpreter::execute(vector<Var *> &) try {
    #ifdef BCPRINT
    Code::FunctionIterator it(this);
    while (it.hasNext()) {
        auto f = (BytecodeFunction *)it.next();
        std::cout << f->id() << ": " << f->name() << std::endl;
        f->bytecode()->dump(std::cout);
        std::cout << std::endl;
    }
    return Status::Ok();
    #endif
    processCall(0);
    while (pointer < bc->length()) {
        Instruction instruction = (Instruction)bc->get(pointer++);
        switch (instruction) {
            case BC_INVALID: throw "Invalid code";
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
            case BC_DPRINT:     cout << pop_return().d; break;
            case BC_IPRINT:     cout << pop_return().i; break;
            case BC_SPRINT:     cout << constantById(pop_return().ui16); break;
            case BC_I2D:        operandStack.back().d = operandStack.back().i; break;
            case BC_D2I :       operandStack.back().i = operandStack.back().d; break;
            case BC_SWAP: {
                    Value upper = pop_return();
                    Value lower = pop_return();
                    operandStack.push_back(upper);
                    operandStack.push_back(lower);
                }; break;
            case BC_POP:        operandStack.pop_back(); break;
            case BC_LOADDVAR:
            case BC_LOADIVAR:
            case BC_LOADSVAR:   operandStack.push_back(variableStack[frame->v_offset + bc_read<uint16_t>()]); break;
            case BC_STOREDVAR:
            case BC_STOREIVAR:
            case BC_STORESVAR:  variableStack[frame->v_offset + bc_read<uint16_t>()] = pop_return(); break;
            case BC_LOADCTXDVAR:
            case BC_LOADCTXIVAR:
            case BC_LOADCTXSVAR:
                operandStack.push_back(
                    variableStack[
                        ctxOffset[bc_read<uint16_t>()].back() + bc_read<uint16_t>()
                    ]
                );
                break;
            case BC_STORECTXDVAR:
            case BC_STORECTXIVAR:
            case BC_STORECTXSVAR:
                variableStack[
                    ctxOffset[bc_read<uint16_t>()].back() + bc_read<uint16_t>()
                ] = pop_return();
                break;
            case BC_DCMP:       operandStack.push_back(compare<double>(pop_return().d, pop_return().d)); break;
            case BC_ICMP:       operandStack.push_back(compare<int64_t>(pop_return().i, pop_return().i)); break;
            case BC_JA:         pointer += bc->getInt16(pointer); break;
            case BC_IFICMPE:    pointer += pop_return().i == pop_return().i ? bc->getInt16(pointer) : sizeof(int16_t); break;
            case BC_IFICMPNE:   pointer += pop_return().i != pop_return().i ? bc->getInt16(pointer) : sizeof(int16_t); break;
            case BC_IFICMPG:    pointer += pop_return().i >  pop_return().i ? bc->getInt16(pointer) : sizeof(int16_t); break;
            case BC_IFICMPGE:   pointer += pop_return().i >= pop_return().i ? bc->getInt16(pointer) : sizeof(int16_t); break;
            case BC_IFICMPL:    pointer += pop_return().i <  pop_return().i ? bc->getInt16(pointer) : sizeof(int16_t); break;
            case BC_IFICMPLE:   pointer += pop_return().i <= pop_return().i ? bc->getInt16(pointer) : sizeof(int16_t); break;
            case BC_CALL:       processCall(bc_read<uint16_t>()); break;
            case BC_DUMP:       cout << operandStack.back().i << endl; break;
            case BC_STOP:       return Status::Warning("Execution stopped");
            case BC_RETURN:     processReturn(); break;
            case BC_BREAK:      break;
            default: throw "Not implemented"; break;
        }
    }
    return Status::Ok();
} catch (const char *msg) {
    return Status::Error(msg);
}

void BytecodeInterpreter::processCall(uint16_t functionId) {
    auto fun =  (BytecodeFunction *)functionById(functionId);
    frame = new StackFrame {
        frame,
        (uint32_t)variableStack.size(),
        (uint32_t)operandStack.size() - fun->parametersNumber(),
        pointer,
        fun
    };

    bc = frame->function->bytecode();
    pointer = 0;
    ctxOffset[functionId].push_back(frame->v_offset);

    // move arguments to call stack
    auto it = std::prev(operandStack.end(), frame->function->parametersNumber());
    std::move(it, operandStack.end(), std::back_inserter(variableStack));
    operandStack.erase(it, operandStack.end());
    variableStack.resize(variableStack.size() + frame->function->localsNumber());
}

inline void BytecodeInterpreter::processReturn() {
    unique_ptr<StackFrame> leaving(frame);
    // remove locals from stack
    variableStack.resize(leaving->v_offset);

    // reset operand stack to previous state
    // (except for non-void function, in this case last operand, i.e. returned value is pushed onto stack back)
    VarType returnType = leaving->function->returnType();
    if (returnType == VT_VOID) {
        operandStack.resize(leaving->o_offset);
    } else {
        Value returnValue = operandStack.back();
        operandStack.resize(leaving->o_offset);
        operandStack.push_back(returnValue);
    }
    // restore pointer
    pointer = leaving->return_pointer;
    ctxOffset[leaving->function->id()].pop_back();
    frame = leaving->previous;
    // restore bytecode
    bc = frame->function->bytecode();
}
