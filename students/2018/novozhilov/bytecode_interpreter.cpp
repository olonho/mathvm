//
// Created by jetbrains on 18.10.18.
//

#include <mathvm.h>
#include <ast.h>
#include "include/bytecode_interpreter.h"

using namespace mathvm;

Status *mathvm::InterpreterCodeImpl::execute(vector<Var *> &vars) {
    BytecodeInterpreter interpreter(this);
    return interpreter.execute(vars);
}

BytecodeInterpreter::BytecodeInterpreter(Code *code, ostream &out) : _code(code), _out(out), _stack(), _context(nullptr) {}

Status *BytecodeInterpreter::execute(vector<Var *> &vars __attribute__ ((unused))) {
    BytecodeFunction *topFunction = getFunctionByName(AstFunction::top_name);
    executeFunction(topFunction);
    return Status::Ok();
}

void BytecodeInterpreter::executeFunction(BytecodeFunction *function) {
    enterContext(function);
    Bytecode *bytecode = function->bytecode();
    uint32_t length = bytecode->length();
    for (uint32_t i = 0; i < length; ) {
        Instruction instruction = getInstruction(bytecode, i);
        switch (instruction) {
            case BC_INVALID:
                throw std::runtime_error("invalid instruction");
            case BC_DLOAD:
                pushDouble(getDouble(bytecode, i));
                break;
            case BC_ILOAD:
                pushInt(getInt64(bytecode, i));
                break;
            case BC_SLOAD: {
                uint16_t id = getUInt16(bytecode, i);
                pushString(_code->constantById(id));
                break;
            }
            case BC_DLOAD0:
                pushDouble(0.0);
                break;
            case BC_ILOAD0:
                pushInt(0);
                break;
            case BC_SLOAD0:
                pushString("");
                break;
            case BC_DLOAD1:
                pushDouble(1.0);
                break;
            case BC_ILOAD1:
                pushInt(1);
                break;
            case BC_DLOADM1:
                pushDouble(-1.0);
                break;
            case BC_ILOADM1:
                pushInt(-1);
                break;
            case BC_DADD:
                pushDouble(popDouble() + popDouble());
                break;
            case BC_IADD:
                pushInt(popInt() + popInt());
                break;
            case BC_DSUB:
                pushDouble(popDouble() - popDouble());
                break;
            case BC_ISUB:
                pushInt(popInt() - popInt());
                break;
            case BC_DMUL:
                pushDouble(popDouble() * popDouble());
                break;
            case BC_IMUL:
                pushInt(popInt() * popInt());
                break;
            case BC_DDIV:
                pushDouble(popDouble() / popDouble());
                break;
            case BC_IDIV:
                pushInt(popInt() / popInt());
                break;
            case BC_IMOD:
                pushInt(popInt() % popInt());
                break;
            case BC_DNEG:
                pushDouble(-popDouble());
                break;
            case BC_INEG:
                pushInt(-popInt());
                break;
            case BC_IAOR:
                pushInt(popInt() | popInt());
                break;
            case BC_IAAND:
                pushInt(popInt() & popInt());
                break;
            case BC_IAXOR:
                pushInt(popInt() ^ popInt());
                break;
            case BC_IPRINT:
                _out << popInt();
                break;
            case BC_DPRINT:
                _out << popDouble();
                break;
            case BC_SPRINT:
                _out << popString();
                break;
            case BC_I2D:
                pushDouble(static_cast<double>(popInt()));
                break;
            case BC_D2I:
                pushInt(static_cast<int64_t>(popDouble()));
                break;
            case BC_S2I:
                pushInt(static_cast<int64_t>(stoi(popString())));
                break;
            case BC_SWAP: {
                auto a = _stack.top();
                _stack.pop();
                auto b = _stack.top();
                _stack.pop();
                _stack.push(a);
                _stack.push(b);
                break;
            }
            case BC_POP:
                _stack.pop();
                break;
            case BC_LOADDVAR:
            case BC_LOADIVAR:
            case BC_LOADSVAR:
            case BC_LOADCTXDVAR:
            case BC_LOADCTXIVAR:
            case BC_LOADCTXSVAR:
                loadVar(bytecode, instruction, i);
                break;
            case BC_STOREDVAR:
            case BC_STOREIVAR:
            case BC_STORESVAR:
            case BC_STORECTXDVAR:
            case BC_STORECTXIVAR:
            case BC_STORECTXSVAR:
                storeVar(bytecode, instruction, i);
                break;
            case BC_DCMP:
                compare(popDouble(), popDouble());
                break;
            case BC_ICMP:
                compare(popInt(), popInt());
                break;
            case BC_JA:
                jump(bytecode, i);
                break;
            case BC_IFICMPNE:
            case BC_IFICMPE:
            case BC_IFICMPG:
            case BC_IFICMPGE:
            case BC_IFICMPL:
            case BC_IFICMPLE:
                conditionalJump(bytecode, instruction, i);
                break;
            case BC_STOP:
                i = length;
                break;
            case BC_CALL: {
                uint16_t id = getUInt16(bytecode, i);
                auto *calledFunction = dynamic_cast<BytecodeFunction*>(_code->functionById(id));
                executeFunction(calledFunction);
                break;
            }
            case BC_CALLNATIVE:
                // TODO
                break;
            case BC_RETURN:
                i = length;
                break;
            default:
                throw std::runtime_error("unsupported instruction");
        }
    }
    exitContext();
}

BytecodeFunction *BytecodeInterpreter::getFunctionByName(string name) {
    return dynamic_cast<BytecodeFunction*>(_code->functionByName(name));
}

void BytecodeInterpreter::pushInt(int64_t value) {
    _stack.push(StackValue(value));
}

void BytecodeInterpreter::pushDouble(double value) {
    _stack.push(StackValue(value));
}

void BytecodeInterpreter::pushString(string value) {
    _stack.push(StackValue(value));
}

int64_t BytecodeInterpreter::popInt() {
    return popTyped<int64_t>();
}

double BytecodeInterpreter::popDouble() {
    return popTyped<double>();
}

string BytecodeInterpreter::popString() {
    return popTyped<string>();
}

template<class T>
T BytecodeInterpreter::popTyped() {
    StackValue value = _stack.top();
    assert(holds_alternative<T>(value));
    _stack.pop();
    return get<T>(value);
}

void BytecodeInterpreter::enterContext(BytecodeFunction *function) {
    auto *context = new Context(function, _context);
    _context = context;
}

void BytecodeInterpreter::exitContext() {
    assert(_context);
    Context *context = _context;
    _context = _context->getParentContext();
    delete context;
}

void BytecodeInterpreter::loadVar(Bytecode* bytecode, Instruction instruction, uint32_t &i) {
    uint16_t varId = getUInt16(bytecode, i);
    Context *context;
    if (instruction == BC_LOADCTXIVAR || instruction == BC_LOADCTXDVAR || instruction == BC_LOADCTXSVAR) {
        uint16_t contextId = getUInt16(bytecode, i);
        context = _context->getContextById(contextId);
    } else {
        context = _context;
    }
    StackValue var = context->getVarById(varId);

    switch (instruction) {
        case BC_LOADIVAR:
        case BC_LOADCTXIVAR:
            pushInt(get<int64_t>(var));
            break;
        case BC_LOADDVAR:
        case BC_LOADCTXDVAR:
            pushDouble(get<double>(var));
            break;
        case BC_LOADSVAR:
        case BC_LOADCTXSVAR:
            pushString(get<string>(var));
            break;
        default:
            throw std::runtime_error("illegal instruction in load section");
    }
}

void BytecodeInterpreter::storeVar(Bytecode *bytecode, Instruction instruction, uint32_t &i) {
    uint16_t varId = getUInt16(bytecode, i);
    Context *context;
    if (instruction == BC_STORECTXIVAR || instruction == BC_STORECTXDVAR || instruction == BC_STORECTXSVAR) {
        uint16_t contextId = getUInt16(bytecode, i);
        context = _context->getContextById(contextId);
    } else {
        context = _context;
    }

    switch (instruction) {
        case BC_STOREIVAR:
        case BC_STORECTXIVAR:
            context->setVarById(StackValue(popInt()), varId);
            break;
        case BC_STOREDVAR:
        case BC_STORECTXDVAR:
            context->setVarById(StackValue(popDouble()), varId);
            break;
        case BC_STORESVAR:
        case BC_STORECTXSVAR:
            context->setVarById(StackValue(popString()), varId);
            break;
        default:
            throw std::runtime_error("illegal instruction in store section");
    }
}

void BytecodeInterpreter::jump(Bytecode *bytecode, uint32_t &i) {
    int16_t offset = getInt16(bytecode, i);
    i += offset - 2;
}

void BytecodeInterpreter::conditionalJump(Bytecode *bytecode, Instruction instruction, uint32_t &i) {
    int64_t upper = popInt();
    int64_t lower = popInt();
    int16_t offset = getInt16(bytecode, i);

    bool condition;
    switch (instruction) {
        case BC_IFICMPNE:
            condition = upper != lower;
            break;
        case BC_IFICMPE:
            condition = upper == lower;
            break;
        case BC_IFICMPG:
            condition = upper > lower;
            break;
        case BC_IFICMPGE:
            condition = upper >= lower;
            break;
        case BC_IFICMPL:
            condition = upper < lower;
            break;
        case BC_IFICMPLE:
            condition = upper <= lower;
            break;
        default:
            throw std::runtime_error("illegal instruction in jump section");
    }

    if (condition) {
        i += offset - 2;
    }
}

template<class T>
T BytecodeInterpreter::getTyped(Bytecode *bytecode, uint32_t &i) {
    T result = bytecode->getTyped<T>(i);
    i += sizeof(T);
    return result;
}

Instruction BytecodeInterpreter::getInstruction(Bytecode *bytecode, uint32_t &i) {
    return bytecode->getInsn(i++);
}

double BytecodeInterpreter::getDouble(Bytecode *bytecode, uint32_t &i) {
    return getTyped<double>(bytecode, i);
}

int16_t BytecodeInterpreter::getInt16(Bytecode *bytecode, uint32_t &i) {
    return getTyped<int16_t >(bytecode, i);
}

uint16_t BytecodeInterpreter::getUInt16(Bytecode *bytecode, uint32_t &i) {
    return getTyped<uint16_t >(bytecode, i);
}

int32_t BytecodeInterpreter::getInt32(Bytecode *bytecode, uint32_t &i) {
    return getTyped<int32_t >(bytecode, i);
}

int64_t BytecodeInterpreter::getInt64(Bytecode *bytecode, uint32_t &i) {
    return getTyped<int64_t >(bytecode, i);
}

template<class T>
void BytecodeInterpreter::compare(T upper, T lower) {
    if (upper < lower) {
        pushInt(-1);
    } else if (upper > lower) {
        pushInt(1);
    } else {
        pushInt(0);
    }
}