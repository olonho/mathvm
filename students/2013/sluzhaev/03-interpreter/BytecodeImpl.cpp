#include <fstream>
#include <sstream>
#include "BytecodeImpl.h"

using namespace mathvm;

Status* BytecodeImpl::execute(vector<Var*> &vars) {
    try {
        return execute();
    } catch (InterpretationError e) {
        throw new Status(e.getMessage());
    }
}

Status* BytecodeImpl::execute() {
    BytecodeFunction* function = (BytecodeFunction*)functionById(0);
    currentScope = new FunctionScope(function, currentScope);
    while (true) {
        switch (currentScope->function->bytecode()->getInsn(currentScope->ip++)) {
            case BC_INVALID:
                throw InterpretationError("Invalid operation");
            case BC_DLOAD:
                exec_dload(getValue<double>(8));
                break;
            case BC_ILOAD:
                exec_iload(getValue<int64_t>(8));
                break;
            case BC_SLOAD:
                exec_sload(getValue<uint16_t>(2));
                break;
            case BC_DLOAD0:
                exec_dload(0.0);
                break;
            case BC_ILOAD0:
                exec_iload((int64_t)0);
                break;
            case BC_SLOAD0:
                exec_sload((int64_t)0);
                break;
            case BC_DLOAD1:
                 exec_dload(1.0);
                break;
            case BC_ILOAD1:
                exec_iload((int64_t)1);
                break;
            case BC_DLOADM1:
                exec_dload(-1.0);
                break;
            case BC_ILOADM1:
                exec_iload((int64_t)(-1));
                break;
            case BC_IADD:
                processIntegerBinaryOperation(tADD);
                break;
            case BC_ISUB:
                processIntegerBinaryOperation(tSUB);
                break;
            case BC_IMUL:
                processIntegerBinaryOperation(tMUL);
                break;
            case BC_IDIV:
                processIntegerBinaryOperation(tDIV);
                break;
            case BC_IMOD:
                processIntegerBinaryOperation(tMOD);
                break;
            case BC_IAAND:
                processIntegerBinaryOperation(tAAND);
                break;
            case BC_IAOR:
                processIntegerBinaryOperation(tAOR);
                break;
            case BC_IAXOR:
                processIntegerBinaryOperation(tAXOR);
                break;
            case BC_ICMP:
                processIntegerBinaryOperation(tEQ);
                break;
            case BC_INEG: {
                Var variable(VT_INT, "");
                variable.setIntValue(-pop().getIntValue());
                variables.push(variable);
                break;
            }
            case BC_DADD:
                processDoubleBinaryOperation(tADD);
                break;
            case BC_DSUB:
                processDoubleBinaryOperation(tSUB);
                break;
            case BC_DMUL:
                processDoubleBinaryOperation(tMUL);
                break;
            case BC_DDIV:
                processDoubleBinaryOperation(tDIV);
                break;
            case BC_DCMP:
                processDoubleBinaryOperation(tEQ);
                break;
            case BC_DNEG: {
                Var variable(VT_DOUBLE, "");
                variable.setDoubleValue(-pop().getDoubleValue());
                variables.push(variable);
                break;
            }
            case BC_SWAP: {
                Var a = pop();
                Var b = pop();
                variables.push(a);
                variables.push(b);
                break;
            }
            case BC_I2D: {
                Var variable(VT_DOUBLE, "");
                variable.setDoubleValue(pop().getIntValue());
                variables.push(variable);
                break;
            }
            case BC_D2I: {
                Var variable(VT_INT, "");
                variable.setIntValue(static_cast<int64_t>(pop().getDoubleValue()));
                variables.push(variable);
                break;
            }
            case BC_S2I: {
                Var variable(VT_INT, "");
                stringstream ss(pop().getStringValue());
                uint64_t res;
                ss >> res;
                variable.setIntValue(res);
                variables.push(variable);
                break;
            }
            case BC_IPRINT:
                std::cout << pop().getIntValue();
                break;
            case BC_DPRINT:
                std::cout << pop().getDoubleValue();
                break;
            case BC_SPRINT: 
                std::cout << pop().getStringValue();
               break;
            case BC_LOADIVAR0:
            case BC_LOADDVAR0:
            case BC_LOADSVAR0:
                loadVariable(0);
                break;
            case BC_LOADIVAR1:
            case BC_LOADDVAR1:
            case BC_LOADSVAR1:
                loadVariable(1);
               break;
            case BC_LOADIVAR2:
            case BC_LOADDVAR2:
            case BC_LOADSVAR2:
                loadVariable(2);
                break;
            case BC_LOADIVAR3:
            case BC_LOADDVAR3:
            case BC_LOADSVAR3:
                loadVariable(3);
                break;
            case BC_LOADIVAR:
            case BC_LOADDVAR:
            case BC_LOADSVAR:
                loadVariable(getValue<uint16_t>(2));
                break;
            case BC_LOADCTXIVAR:
            case BC_LOADCTXDVAR:
            case BC_LOADCTXSVAR: {
                uint16_t a = getValue<uint16_t>(2); 
                uint16_t b = getValue<uint16_t>(2);
                loadVariable(a, b); 
                break;
            }
            case BC_STOREIVAR0:
            case BC_STOREDVAR0:
            case BC_STORESVAR0:
                storeVariable(0);
                break;
            case BC_STOREIVAR1:
            case BC_STOREDVAR1:
            case BC_STORESVAR1:
                storeVariable(1);
                break;
            case BC_STOREIVAR2:
            case BC_STOREDVAR2:
            case BC_STORESVAR2:
                storeVariable(2);
                break;
            case BC_STOREIVAR3:
            case BC_STOREDVAR3:
            case BC_STORESVAR3:
                 storeVariable(3);
                break;
            case BC_STOREIVAR:
            case BC_STOREDVAR:
            case BC_STORESVAR:
                storeVariable(getValue<uint16_t>(2));
                break;
            case BC_STORECTXIVAR:
            case BC_STORECTXDVAR:
            case BC_STORECTXSVAR: {
                uint16_t a = getValue<uint16_t>(2); 
                uint16_t b = getValue<uint16_t>(2); 
                storeVariable(a, b); 
                break;
            }
            case BC_JA:
                currentScope->ip += currentScope->function->bytecode()->getInt16(currentScope->ip);
                break;
            case BC_IFICMPE: {
                int64_t a = pop().getIntValue();
                int64_t b = pop().getIntValue();
                currentScope->ip += (a == b) ? currentScope->function->bytecode()->getInt16(currentScope->ip) : 2;
                break;
            }
            case BC_IFICMPNE: {
                int64_t a = pop().getIntValue();
                int64_t b = pop().getIntValue();
                currentScope->ip += (a != b) ? currentScope->function->bytecode()->getInt16(currentScope->ip) : 2;
                break;
            }
            case BC_IFICMPG: {
                int64_t a = pop().getIntValue();
                int64_t b = pop().getIntValue();
                currentScope->ip += (a > b) ? currentScope->function->bytecode()->getInt16(currentScope->ip) : 2;
                break;
            }
            case BC_IFICMPL: {
                int64_t a = pop().getIntValue();
                int64_t b = pop().getIntValue();
                currentScope->ip += (a < b) ? currentScope->function->bytecode()->getInt16(currentScope->ip) : 2;
                break;
            }
            case BC_IFICMPGE: {
                int64_t a = pop().getIntValue();
                int64_t b = pop().getIntValue();
                currentScope->ip += (a >= b) ? currentScope->function->bytecode()->getInt16(currentScope->ip) : 2;
                break;
            }
            case BC_IFICMPLE: {
                int64_t a = pop().getIntValue();
                int64_t b = pop().getIntValue();
                currentScope->ip += (a <= b) ? currentScope->function->bytecode()->getInt16(currentScope->ip) : 2;
                break;
            }
            case BC_CALL: {
                BytecodeFunction* function = (BytecodeFunction*)functionById(getValue<uint16_t>(2));
                currentScope = new FunctionScope(function, currentScope);
                break;
            }
            case BC_CALLNATIVE:
                // TODO
                break;
            case BC_RETURN: {
                FunctionScope* parent = currentScope->parent;
                delete currentScope;
                currentScope = parent;
                break;
            }
            case BC_STOP:
                return 0;
            default:
                throw InterpretationError("Unknown instruction");
        }
    }
    return 0;
}

Var BytecodeImpl::pop() {
    if (variables.empty()) {
        throw std::string("Stack is empty");
    }
    Var res = variables.top();
    variables.pop();
    return res;
}

void BytecodeImpl::loadVariable(uint16_t id) {
    variables.push(currentScope->variables[id]);
}

void BytecodeImpl::loadVariable(uint16_t cid, uint16_t id) {
    FunctionScope* scope = currentScope->parent;
    while (scope && scope->function->id() != cid) {
        scope = scope->parent;
    }
    if (!scope) {
        throw InterpretationError("Context not found");
    }
    variables.push(scope->variables[id]);
}

void BytecodeImpl::storeVariable(uint16_t id) {
    currentScope->variables[id] = pop();
}

void BytecodeImpl::storeVariable(uint16_t cid, uint16_t id) {
    FunctionScope* scope = currentScope->parent;
    while (scope && scope->function->id() != cid) {
        scope = scope->parent;
    }
    if (!scope) {
        throw InterpretationError("context not found");
    }
    scope->variables[id] = pop();
}

void BytecodeImpl::exec_dload(double val) {
    Var v(VT_DOUBLE, "");
    v.setDoubleValue(val);
    variables.push(v);
}

void BytecodeImpl::exec_iload(int64_t val) {
    Var v(VT_INT, "");
    v.setIntValue(val);
    variables.push(v);
}

void BytecodeImpl::exec_sload(uint16_t id) {
    Var v(VT_STRING, "");
    v.setStringValue(constantById(id).data());
    variables.push(v);
}

void BytecodeImpl::processDoubleBinaryOperation(TokenKind op) {
    Var a = pop();
    Var b = pop();
    Var res(VT_DOUBLE, "");
    switch (op) {
        case tADD:
            res.setDoubleValue(a.getDoubleValue() + b.getDoubleValue());
            break;
        case tSUB:
            res.setDoubleValue(a.getDoubleValue() - b.getDoubleValue());
            break;
        case tMUL:
            res.setDoubleValue(a.getDoubleValue() * b.getDoubleValue());
            break;
        case tDIV:
            res.setDoubleValue(a.getDoubleValue() / b.getDoubleValue());
            break;
        case tEQ:
            res.setDoubleValue((int64_t)(a.getDoubleValue() == b.getDoubleValue()));
            break;
        default:
            throw InterpretationError("Unsupported double binary operation");
    }
    variables.push(res);
}

void BytecodeImpl::processIntegerBinaryOperation(TokenKind op) {
    Var a = pop();
    Var b = pop();
    Var res(VT_INT, "");
    switch (op) {
        case tADD:
            res.setIntValue(a.getIntValue() + b.getIntValue());
            break;
        case tSUB:
            res.setIntValue(a.getIntValue() - b.getIntValue());
            break;
        case tMUL:
            res.setIntValue(a.getIntValue() * b.getIntValue());
            break;
        case tDIV:
            res.setIntValue(a.getIntValue() / b.getIntValue());
            break;
        case tMOD:
            res.setIntValue(a.getIntValue() % b.getIntValue());
            break;
        case tAAND:
            res.setIntValue(a.getIntValue() & b.getIntValue());
            break;
        case tAOR:
            res.setIntValue(a.getIntValue() | b.getIntValue());
            break;
        case tAXOR:
            res.setIntValue(a.getIntValue() ^ b.getIntValue());
            break;
        case tEQ:
            res.setIntValue((int64_t)(a.getIntValue() == b.getIntValue()));
            break;
        default:
            throw InterpretationError("Unsupported integer binary operation");
    }
    variables.push(res);
}
