//
// Created by Владислав Калинин on 17/11/2018.
//

#include "BytecodeInterpeter.h"

using namespace mathvm;

Status *BytecodeInterpeter::execute(std::vector<Var *> &vars) {
    while (true) {
        Instruction ins = shiftAndGetInstruction();
        switch (ins) {
            case BC_INVALID:
                return Status::Error("Runtime Error", callStack->offset);
            case BC_DLOAD:
                pushDouble(shiftAndGetValue<double>());
                break;
            case BC_ILOAD:
                pushInt64(shiftAndGetValue<int64_t>());
                break;
            case BC_SLOAD:
                pushInt16(shiftAndGetValue<uint16_t>());
                break;
            case BC_DLOAD0:
                pushDouble(0);
                break;
            case BC_ILOAD0:
                pushInt64(0);
                break;
            case BC_SLOAD0:
                pushInt16(0);
                break;
            case BC_DLOAD1:
                pushDouble(1);
                break;
            case BC_ILOAD1:
                pushInt64(1);
                break;
            case BC_DLOADM1:
                pushDouble(-1);
                break;
            case BC_ILOADM1:
                pushInt64(-1);
                break;
            case BC_DADD:
                evalDoubleExpession(ADD);
                break;
            case BC_IADD:
                evalIntegerExpession(ADD);
                break;
            case BC_DSUB:
                evalDoubleExpession(SUB);
                break;
            case BC_ISUB:
                evalIntegerExpession(SUB);
                break;
            case BC_DMUL:
                evalDoubleExpession(MUL);
                break;
            case BC_IMUL:
                evalIntegerExpession(MUL);
                break;
            case BC_DDIV:
                evalDoubleExpession(DIV);
                break;
            case BC_IDIV:
                evalIntegerExpession(DIV);
                break;
            case BC_IMOD:
                evalIntegerExpession(MOD);
                break;
            case BC_DNEG:
                pushDouble(-popDouble());
                break;
            case BC_INEG:
                pushInt64(-popInt64());
                break;
            case BC_IAOR:
                evalIntegerExpession(OR);
                break;
            case BC_IAAND:
                evalIntegerExpession(AND);
                break;
            case BC_IAXOR:
                evalIntegerExpession(XOR);
                break;
            case BC_IPRINT:
                cout << popInt64();
                break;
            case BC_DPRINT:
                cout << popDouble();
                break;
            case BC_SPRINT:
                cout << globalCtx->getStringConstantById(popInt16());
                break;
            case BC_I2D:
                pushDouble(popInt64());
                break;
            case BC_D2I:
                pushInt64(static_cast<int64_t>(popDouble()));
                break;
            case BC_S2I:
                pushInt64(popInt16());
                break;
            case BC_SWAP:
                //TODO
                break;
            case BC_POP:
                stack.pop_back();
                break;
            case BC_LOADDVAR0:
            case BC_LOADDVAR1:
            case BC_LOADDVAR2:
            case BC_LOADDVAR3:
            case BC_LOADIVAR0:
            case BC_LOADIVAR1:
            case BC_LOADIVAR2:
            case BC_LOADIVAR3:
            case BC_LOADSVAR0:
            case BC_LOADSVAR1:
            case BC_LOADSVAR2:
            case BC_LOADSVAR3:
                //TODO
                break;
            case BC_STOREDVAR0:
            case BC_STOREDVAR1:
            case BC_STOREDVAR2:
            case BC_STOREDVAR3:
            case BC_STOREIVAR0:
            case BC_STOREIVAR1:
            case BC_STOREIVAR2:
            case BC_STOREIVAR3:
            case BC_STORESVAR0:
            case BC_STORESVAR1:
            case BC_STORESVAR2:
            case BC_STORESVAR3:
                //TODO
                break;
            case BC_LOADDVAR:
                pushDouble(ctx->getDouble(shiftAndGetValue<uint16_t>()));
                break;
            case BC_LOADIVAR:
                pushInt64(ctx->getInt64(shiftAndGetValue<uint16_t>()));
                break;
            case BC_LOADSVAR:
                pushInt16(ctx->getInt16(shiftAndGetValue<uint16_t>()));
                break;
            case BC_STOREDVAR:
                ctx->setDouble(shiftAndGetValue<uint16_t>(), popDouble());
                break;
            case BC_STOREIVAR:
                ctx->setInt64(shiftAndGetValue<uint16_t>(), popInt64());
                break;
            case BC_STORESVAR:
                ctx->setInt16(shiftAndGetValue<uint16_t>(), popInt16());
                break;
            case BC_LOADCTXDVAR:
                loadValueFromUpperContext(VT_DOUBLE);
                break;
            case BC_LOADCTXIVAR:
                loadValueFromUpperContext(VT_INT);
                break;
            case BC_LOADCTXSVAR:
                loadValueFromUpperContext(VT_STRING);
                break;
            case BC_STORECTXDVAR:
                storeValueToUpperContext(VT_DOUBLE);
                break;
            case BC_STORECTXIVAR:
                storeValueToUpperContext(VT_INT);
                break;
            case BC_STORECTXSVAR:
                storeValueToUpperContext(VT_STRING);
                break;
            case BC_DCMP:
                compare(VT_DOUBLE);
                break;
            case BC_ICMP:
                compare(VT_INT);
                break;
            case BC_JA:
                callStack->offset += shiftAndGetValue<int16_t>() - 2;
                break;
            case BC_IFICMPNE:
                evalIfExpression(NEQ);
                break;
            case BC_IFICMPE:
                evalIfExpression(EQ);
                break;
            case BC_IFICMPG:
                evalIfExpression(GT);
                break;
            case BC_IFICMPGE:
                evalIfExpression(GE);
                break;
            case BC_IFICMPL:
                evalIfExpression(LT);
                break;
            case BC_IFICMPLE:
                evalIfExpression(LE);
                break;
            case BC_DUMP:
                //TODO
                break;
            case BC_STOP:
                return Status::Ok();
            case BC_CALL:
                callFunction();
                break;
            case BC_CALLNATIVE:
                //TODO
                break;
            case BC_RETURN:
                returnFromFunction();
                break;
            case BC_BREAK:
                //TODO
                break;
            default:
                break;
        }
    }
}

void BytecodeInterpeter::init(Bytecode *bytecode) {
    ctx = new StackContext(globalCtx->varNumber());
    callStack = new CallStack(bytecode);
}

template<class T>
T BytecodeInterpeter::shiftAndGetValue() {
    T value = callStack->bytecode->getTyped<T>(callStack->offset);
    callStack->offset += sizeof(T);
    return value;
}

Instruction BytecodeInterpeter::shiftAndGetInstruction() {
    Instruction ins = callStack->bytecode->getInsn(callStack->offset);
    callStack->offset += 1;
    return ins;
}

void BytecodeInterpeter::pushInt64(int64_t value) {
    stack.push_back(value);
}

void BytecodeInterpeter::pushInt16(uint16_t value) {
    stack.push_back(value);
}

void BytecodeInterpeter::pushDouble(double value) {
    stack.push_back(value);
}

int64_t BytecodeInterpeter::popInt64() {
    int64_t value = stack.back().i;
    stack.pop_back();
    return value;
}

uint16_t BytecodeInterpeter::popInt16() {
    uint16_t value = stack.back().i16;
    stack.pop_back();
    return value;
}

double BytecodeInterpeter::popDouble() {
    double value = stack.back().d;
    stack.pop_back();
    return value;
}

void BytecodeInterpeter::evalIntegerExpession(ArithmeticOperation op) {
    int64_t right = popInt64();
    int64_t left = popInt64();
    int64_t result = 0;
    switch (op) {
        case OR:
            result = left | right;
            break;
        case AND:
            result = left & right;
            break;
        case XOR:
            result = left ^ right;
            break;
        case ADD:
            result = left + right;
            break;
        case SUB:
            result = left - right;
            break;
        case MUL:
            result = left * right;
            break;
        case DIV:
            result = left / right;
            break;
        case MOD:
            result = left % right;
            break;
    }
    pushInt64(result);
}

void BytecodeInterpeter::evalDoubleExpession(ArithmeticOperation op) {
    double right = popDouble();
    double left = popDouble();
    double result = 0;
    switch (op) {
        case ADD:
            result = left + right;
            break;
        case SUB:
            result = left - right;
            break;
        case MUL:
            result = left * right;
            break;
        case DIV:
            result = left / right;
            break;
        default:
            break;

    }
    pushDouble(result);
}

void BytecodeInterpeter::callFunction() {
    uint16_t funcId = shiftAndGetValue<uint16_t>();
    BytecodeFunction *func = globalCtx->getFunctiontById(funcId);
    callStack = new CallStack(func->bytecode(), callStack);
    ctx = new StackContext(func, ctx);
}

void BytecodeInterpeter::returnFromFunction() {
    CallStack *currentCallStack = callStack;
    StackContext *currentContext = ctx;
    callStack = currentCallStack->prev;
    ctx = currentContext->previousContext();
    delete currentCallStack;
    delete currentContext;
}

void BytecodeInterpeter::loadValueFromUpperContext(VarType type) {
    uint16_t contextId = shiftAndGetValue<uint16_t>();
    uint16_t variableId = shiftAndGetValue<uint16_t>();
    switch (type) {
        case VT_DOUBLE:
            pushDouble(ctx->getDoubleFromParent(contextId, variableId));
            break;
        case VT_INT:
            pushInt64(ctx->getInt64FromParent(contextId, variableId));
            break;
        case VT_STRING:
            pushInt16(ctx->getInt16FromParent(contextId, variableId));
            break;
        default:
            break;
    }
}

void BytecodeInterpeter::storeValueToUpperContext(VarType type) {
    uint16_t contextId = shiftAndGetValue<uint16_t>();
    uint16_t variableId = shiftAndGetValue<uint16_t>();
    switch (type) {
        case VT_DOUBLE:
            ctx->setDoubleToParent(contextId, variableId, popDouble());
            break;
        case VT_INT:
            ctx->setInt64ToParent(contextId, variableId, popInt64());
            break;
        case VT_STRING:
            ctx->setInt16ToParent(contextId, variableId, popInt16());
            break;
        default:
            break;
    }
}

void BytecodeInterpeter::compare(VarType type) {
    double left = 0, right = 0;
    switch (type) {
        case VT_DOUBLE:
            right = popDouble();
            left = popDouble();
            break;
        case VT_INT:
            right = popInt64();
            left = popInt64();
        default:
            break;
    }
    if (left == right) {
        pushInt64(0);
    } else if (left < right) {
        pushInt64(-1);
    } else {
        pushInt64(1);
    }
}

void BytecodeInterpeter::evalIfExpression(CompareOperation op) {
    int64_t right = popInt64();
    int64_t left = popInt64();
    int res = false;
    switch (op) {
        case EQ:
            res = left == right;
            break;
        case NEQ:
            res = left != right;
            break;
        case GT:
            res = left > right;
            break;
        case GE:
            res = left >= right;
            break;
        case LT:
            res = left < right;
            break;
        case LE:
            res = left <= right;
            break;
    }
    int16_t labelOffset = shiftAndGetValue<int16_t>();
    if (!res) {
        callStack->offset += labelOffset - 2;
    }
}
