#include "bytecodeinterpreter.hpp"

#include <iostream>
#include <functional>
#include <cmath>
using std::cout;
using std::showpoint;
using std::endl;

//std::plus<double> dadd;
//std::plus<int64_t> iadd;
//std::minus<double> dsub;
//std::minus<int64_t> isub;
//std::multiplies<double> dmult;
//std::multiplies<int64_t> imult;
//std::divides<double> ddiv;
//std::divides<int64_t> idiv;
//std::modulus<int64_t> imod;

template <typename T>
int64_t compare(T left, T right) {
    if (left == right) {
        return 0;
    } else if (left < right) {
        return -1;
    } else  {
        return 1;
    }
}

Status *BytecodeInterpreter::execute(vector<Var *> &) try {
    /*
        Code::FunctionIterator it(this);
        while (it.hasNext()) {
            auto f = (BytecodeFunction *)it.next();
            std::cout << f->id() << ": " << f->name() << std::endl;
            f->bytecode()->dump(std::cout);
            std::cout << std::endl;
        }
    */
    processCall(0);
    while (pointer < bc->length()) {
        Instruction instruction = (Instruction)bc->get(pointer++);
        switch (instruction) {
            case BC_INVALID: throw "Invalid code";
            case BC_DLOAD: {
                    operandStack.push_back(Value(bc->getDouble(pointer)));
                    pointer += sizeof(double);
                }; break;
            case BC_ILOAD: {
                    operandStack.push_back(Value(bc->getInt64(pointer)));
                    pointer += sizeof(int64_t);
                }; break;
            case BC_SLOAD:
                operandStack.push_back(Value(bc->getUInt16(pointer)));
                pointer += sizeof(uint16_t);
                break;
            case BC_DLOAD0: operandStack.push_back((double)0); break;
            case BC_ILOAD0: operandStack.push_back((int64_t)0); break;
            case BC_SLOAD0: operandStack.push_back((uint16_t)0); break;
            case BC_DLOAD1: operandStack.push_back((double)1.0); break;
            case BC_ILOAD1: operandStack.push_back((int64_t)1); break;
            case BC_DLOADM1: operandStack.push_back((double) - 1.0); break;
            case BC_ILOADM1: operandStack.push_back((int64_t) - 1); break;
            case BC_DADD:
            case BC_IADD:
            case BC_DSUB:
            case BC_ISUB:
            case BC_DMUL:
            case BC_IMUL:
            case BC_DDIV:
            case BC_IDIV:
            case BC_IMOD:
            case BC_IAOR:
            case BC_IAAND:
            case BC_IAXOR:  processBinaryOperation(instruction); break;
            case BC_DNEG:
            case BC_INEG:   processUnaryOperaion(instruction); break;
            case BC_DPRINT:
            case BC_IPRINT:
            case BC_SPRINT: processPrint(instruction); break;
            case BC_I2D: {
                    Value v = operandStack.back();
                    v._d =  v._i;
                    operandStack.push_back(v);
                }; break;
            case BC_D2I : {
                    Value v = operandStack.back();
                    v._i =  v._d;
                    operandStack.push_back(v);
                }; break;
            case BC_SWAP: {
                    Value upper = operandStack.back();
                    operandStack.pop_back();
                    Value lower = operandStack.back();
                    operandStack.pop_back();
                    operandStack.push_back(upper);
                    operandStack.push_back(lower);
                }; break;
            case BC_POP: operandStack.pop_back(); break;
            case BC_LOADCTXDVAR:
            case BC_LOADCTXIVAR:
            case BC_LOADCTXSVAR: {
                    uint16_t ctxID = bc->getUInt16(pointer);
                    pointer += sizeof(uint16_t);
                    uint16_t varID = bc->getUInt16(pointer);
                    pointer += sizeof(uint16_t);
                    operandStack.push_back(variableStack[findVarIndex(ctxID, varID)]);
                }; break;
            case BC_STORECTXDVAR:
            case BC_STORECTXIVAR:
            case BC_STORECTXSVAR: {
                    uint16_t ctxID = bc->getUInt16(pointer);
                    pointer += sizeof(uint16_t);
                    uint16_t varID = bc->getUInt16(pointer);
                    pointer += sizeof(uint16_t);
                    variableStack[findVarIndex(ctxID, varID)] = operandStack.back();
                    operandStack.pop_back();
                }; break;
            case BC_DCMP: {
                    Value upper = operandStack.back();
                    operandStack.pop_back();
                    Value lower = operandStack.back();
                    operandStack.pop_back();
                    operandStack.push_back(compare<double>(upper._d, lower._d));
                }; break;
            case BC_ICMP: {
                    Value upper = operandStack.back();
                    operandStack.pop_back();
                    Value lower = operandStack.back();
                    operandStack.pop_back();
                    operandStack.push_back(compare<int64_t>(upper._i, lower._i));
                }; break;
            case BC_JA:         pointer += bc->getInt16(pointer); break;
            case BC_IFICMPE:
            case BC_IFICMPNE:
            case BC_IFICMPG:
            case BC_IFICMPGE:
            case BC_IFICMPL:
            case BC_IFICMPLE:   processConditionalJump(instruction); break;
            case BC_DUMP:       cout << operandStack.back()._i << endl; break;
            case BC_STOP:       return Status::Warning("Execution stopped");
            case BC_CALL: {
                    uint16_t functionId = bc->getUInt16(pointer);
                    pointer += sizeof(uint16_t);
                    processCall(functionId);
                }; break;
            case BC_RETURN: {

                }
            default: break;
        }
    }
    return Status::Ok();
} catch (const char *msg) {
    return Status::Error(msg);
}

void BytecodeInterpreter::processCall(uint16_t functionId) {
    auto fun = (BytecodeFunction *)functionById(functionId);

    frame = new StackFrame {frame, functionId, (uint32_t)variableStack.size(), pointer};

    auto params = fun->parametersNumber();
    for (auto i = 0; i < params; ++i) {
        variableStack.push_back(*(operandStack.end() - params + i));
    }
    for (auto i = 0; i < params; ++i) {
        operandStack.pop_back();
    }

    auto locals = fun->localsNumber();
    for (auto i = 0; i < locals; ++i) {
        variableStack.push_back(Value());
    }

    bc = fun->bytecode();
    pointer = 0;
}

void BytecodeInterpreter::processReturn() {
    while (variableStack.size() != frame->offset) {
        variableStack.pop_back();
    }
    frame = frame->parent;
    auto fun = (BytecodeFunction *)functionById(frame->functionId);
    bc = fun->bytecode();
    pointer = frame->pointer;
}

void BytecodeInterpreter::processBinaryOperation(Instruction instruction) {
    Value right = operandStack.back();
    operandStack.pop_back();
    Value left = operandStack.back();
    operandStack.pop_back();
    Value result;
    switch (instruction) {
        case BC_DADD:   result = left._d + right._d; break;
        case BC_IADD:   result = left._i + right._i; break;
        case BC_DSUB:   result = left._d - right._d; break;
        case BC_ISUB:   result = left._i - right._i; break;
        case BC_DMUL:   result = left._d * right._d; break;
        case BC_IMUL:   result = left._i * right._i; break;
        case BC_DDIV:   result = left._d / right._d; break;
        case BC_IDIV:   result = left._i / right._i; break;
        case BC_IMOD:   result = left._i % right._i; break;
        case BC_IAOR:   result = left._i | right._i; break;
        case BC_IAAND:  result = left._i & right._i; break;
        case BC_IAXOR:  result = left._i ^ right._i; break;
        default: throw "Error processing binaryOperation";
    }
    operandStack.push_back(result);
}

void BytecodeInterpreter::processUnaryOperaion(Instruction instruction) {
    Value operand = operandStack.back();
    operandStack.pop_back();
    Value result;
    switch (instruction) {
        case BC_DNEG: result = -operand._d; break;
        case BC_INEG: result = -operand._i; break;
        default: throw "Error processing unary operation";
    }
    operandStack.push_back(result);
}

void BytecodeInterpreter::processPrint(Instruction instruction) {
    Value operand = operandStack.back();
    operandStack.pop_back();
    switch (instruction) {
        case BC_DPRINT: cout << operand._d; break;
        case BC_IPRINT: cout << operand._i; break;
        case BC_SPRINT: cout << constantById(operand._ui); break;
        default: throw "Error processing print";
    }
}

void BytecodeInterpreter::processConditionalJump(Instruction instruction) {
    Value upper = operandStack.back();
    operandStack.pop_back();
    Value lower = operandStack.back();
    operandStack.pop_back();
    int16_t offset = bc->getInt16(pointer);
    switch (instruction) {
        case BC_IFICMPE:    pointer += upper._i == lower._i ? offset : sizeof(int16_t); break;
        case BC_IFICMPNE:   pointer += upper._i != lower._i ? offset : sizeof(int16_t); break;
        case BC_IFICMPG:    pointer += upper._i >  lower._i ? offset : sizeof(int16_t); break;
        case BC_IFICMPGE:   pointer += upper._i >= lower._i ? offset : sizeof(int16_t); break;
        case BC_IFICMPL:    pointer += upper._i <  lower._i ? offset : sizeof(int16_t); break;
        case BC_IFICMPLE:   pointer += upper._i <= lower._i ? offset : sizeof(int16_t); break;
        default: throw "Error processing conditional jump";
    }
}

uint32_t BytecodeInterpreter::findVarIndex(uint16_t ctxID, uint16_t varID) {
    StackFrame *current = frame;
    while (current->functionId != ctxID) {
        current = current->parent;
    }
    return current->offset + varID;
}
