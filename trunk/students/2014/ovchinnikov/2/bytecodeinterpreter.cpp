#include "bytecodeinterpreter.hpp"

#include <functional>

std::multiplies<double> dmult;
std::multiplies<int64_t> imult;

Status *BytecodeInterpreter::execute(vector<Var *> &) {
    Code::FunctionIterator it(this);
    while (it.hasNext()) {
        auto f = (BytecodeFunction *)it.next();
        std::cout << f->id() << ": " << f->name() << std::endl;
        f->bytecode()->dump(std::cout);
        std::cout << std::endl;
    }

    return Status::Ok();
}

void BytecodeInterpreter::processInstruction(Instruction instruction) {
    switch (pointer) {
        case BC_INVALID: throw Status::Error("Invalid code");
        case BC_DLOAD:  operandStack.push_back(bc->getDouble(pointer++)); break;
        case BC_ILOAD:  operandStack.push_back(bc->getInt64(pointer++)); break;
        case BC_SLOAD:  operandStack.push_back(bc->getInt16(pointer++)); break;
        case BC_DLOAD0: operandStack.push_back((double)0); break;
        case BC_ILOAD0: operandStack.push_back((int64_t)0); break;
        case BC_SLOAD0: operandStack.push_back((int16_t)0); break;
        case BC_DLOAD1: operandStack.push_back((double)1.0); break;
        case BC_ILOAD1: operandStack.push_back((int64_t)1); break;
        case BC_DLOADM1: operandStack.push_back((double) - 1.0); break;
        case BC_ILOADM1: operandStack.push_back((int64_t) - 1); break;
        case BC_DADD: processArithmeticOperation(instruction);
    }
}



void BytecodeInterpreter::processArithmeticOperation(Instruction instruction) {
    uint64_t right = operandStack.back();
    operandStack.pop_back();
    uint64_t left = operandStack.back();
    operandStack.pop_back();
    uint64_t result;
    switch (instruction) {
        case BC_DADD:   result = (double) left + (double) right; break;
        case BC_IADD:   result = (int64_t) left + (int64_t) right; break;
        case BC_DSUB:   result = (double) left - (double) right;
        case BC_ISUB:   result = (int64_t) left - (int64_t) right;
        case BC_DMUL:   result = dmult(left, right);
        case BC_IMUL:   result = imult(left, right);
        default:
            break;
    }
}
