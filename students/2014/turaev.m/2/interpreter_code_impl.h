#ifndef INTERPRETER_CODE_IMPL_H
#define INTERPRETER_CODE_IMPL_H

#include "mathvm.h"
#include <stack>
#include <iostream>

using namespace std;

namespace mathvm {
class InterpreterCodeImpl: public Code {
    map<Instruction, uint32_t> instrSize;
    stack<uint64_t> argsStack;
    map<uint16_t, stack<vector<uint64_t> > > vars; // fun_id -> stack [value]

    template <typename T>
    void push(T val) {
        argsStack.push((uint64_t)val);
    }

    template <typename T>
    T top() {
        T res = (T)argsStack.top();
        return res;
    }

    void pop() {
        argsStack.pop();
    }

    void setSizes() {
#define SET_SIZE(b, d, l) instrSize[BC_##b] = l;
        FOR_BYTECODES(SET_SIZE);
#undef SET_SIZE
    }

public:
    virtual Status *execute(vector<Var *> &) {
        setSizes();

        BytecodeFunction *mainFunction = dynamic_cast<BytecodeFunction *>(this->functionById(0));
        vars[0].push(vector<uint64_t>(mainFunction->localsNumber()));
        executeFunction(mainFunction);
        return Status::Ok();
    }

    void executeFunction(BytecodeFunction *function) {
        uint32_t ip = 0;
        Bytecode *bytecode = function->bytecode();
        uint16_t functionId = function->id();
        while (ip != bytecode->length()) {
            Instruction current = bytecode->getInsn(ip);

            switch (current) {
                //TODO assert that stack same size before and after execution
                case BC_DLOAD: {
                    push(bytecode->getDouble(ip + 1));
                    break;
                }
                case BC_ILOAD: {
                    push(bytecode->getInt64(ip + 1));
                    break;
                }
                case BC_SLOAD: {
                    push(bytecode->getUInt16(ip + 1));
                    break;
                }
                case BC_ILOAD0: {
                    push(0);
                    break;
                }
                case BC_ILOAD1: {
                    push(1);
                    break;
                }
                case BC_DADD: {
                    double left = top<double>();
                    pop();
                    double right = top<double>();
                    pop();
                    push(left + right);
                    break;
                }
                case BC_IADD: {
                    int64_t left = top<int64_t>();
                    pop();
                    int64_t right = top<int64_t>();
                    pop();
                    push(left + right);
                    break;
                }
                case BC_DSUB: {
                    double left = top<double>();
                    pop();
                    double right = top<double>();
                    pop();
                    push(left - right);
                    break;
                }
                case BC_ISUB: {
                    int64_t left = top<int64_t>();
                    pop();
                    int64_t right = top<int64_t>();
                    pop();
                    push(left - right);
                    break;
                }
                case BC_DMUL: {
                    double left = top<double>();
                    pop();
                    double right = top<double>();
                    pop();
                    push(left * right);
                    break;
                }
                case BC_IMUL: {
                    int64_t left = top<int64_t>();
                    pop();
                    int64_t right = top<int64_t>();
                    pop();
                    push(left * right);
                    break;
                }
                case BC_DDIV: {
                    double left = top<double>();
                    pop();
                    double right = top<double>();
                    pop();
                    push(left / right);
                    break;
                }
                case BC_IDIV: {
                    int64_t left = top<int64_t>();
                    pop();
                    int64_t right = top<int64_t>();
                    pop();
                    push(left / right);
                    break;
                }
                case BC_IMOD: {
                    int64_t left = top<int64_t>();
                    pop();
                    int64_t right = top<int64_t>();
                    pop();
                    push(left % right);
                    break;
                }
                case BC_DNEG: {
                    double left = top<double>();
                    pop();
                    push(-left);
                    break;
                }
                case BC_INEG: {
                    int64_t left = top<int64_t>();
                    pop();
                    push(-left);
                    break;
                }
                case BC_IAOR: {
                    int64_t left = top<int64_t>();
                    pop();
                    int64_t right = top<int64_t>();
                    pop();
                    push(left | right);
                    break;
                }
                case BC_IAAND: {
                    int64_t left = top<int64_t>();
                    pop();
                    int64_t right = top<int64_t>();
                    pop();
                    push(left & right);
                    break;
                }
                case BC_IAXOR: {
                    int64_t left = top<int64_t>();
                    pop();
                    int64_t right = top<int64_t>();
                    pop();
                    push(left ^ right);
                    break;
                }
                case BC_IPRINT: {
                    int64_t arg = top<int64_t>();
                    pop();
                    cout << arg;
                    break;
                }
                case BC_DPRINT: {
                    double arg = top<double>();
                    pop();
                    cout << arg;
                    break;
                }
                case BC_SPRINT: {
                    uint16_t arg = top<uint16_t>();
                    pop();
                    cout << constantById(arg);
                    break;
                }
                case BC_I2D: {
                    int64_t arg = top<int64_t>();
                    pop();
                    push((double)arg);
                    break;
                }
                case BC_D2I: {
                    double arg = top<double>();
                    pop();
                    push((int64_t)arg);
                    break;
                }
                case BC_SWAP: {
                    uint64_t arg1 = top<uint64_t>();
                    pop();
                    uint64_t arg2 = top<uint64_t>();
                    pop();
                    push(arg1);
                    push(arg2);
                    break;
                }
                case BC_POP: {
                    pop();
                    break;
                }
                case BC_LOADDVAR: {
                    uint16_t varId = bytecode->getUInt16(ip + 1);
                    push(vars[functionId].top()[varId]);
                    break;
                }
                case BC_LOADIVAR: {
                    uint16_t varId = bytecode->getUInt16(ip + 1);
                    push(vars[functionId].top()[varId]);
                    break;
                }
                case BC_LOADSVAR: {
                    uint16_t varId = bytecode->getUInt16(ip + 1);
                    push(vars[functionId].top()[varId]);
                    break;
                }
                case BC_STOREDVAR: {
                    uint16_t varId = bytecode->getUInt16(ip + 1);
                    double value = top<double>();
                    pop();
                    vars[functionId].top()[varId] = value;
                    break;
                }
                case BC_STOREIVAR: {
                    uint16_t varId = bytecode->getUInt16(ip + 1);
                    int64_t value = top<int64_t>();
                    pop();
                    vars[functionId].top()[varId] = value;
                    break;
                }
                case BC_STORESVAR: {
                    uint16_t varId = bytecode->getUInt16(ip + 1);
                    uint16_t value = top<uint16_t>();
                    pop();
                    vars[functionId].top()[varId] = value;
                    break;
                }
                case BC_LOADCTXDVAR: {
                    uint16_t contextId = bytecode->getUInt16(ip + 1);
                    uint16_t varId = bytecode->getUInt16(ip + 3);
                    push(vars[contextId].top()[varId]);
                    break;
                }
                case BC_LOADCTXIVAR: {
                    uint16_t contextId = bytecode->getUInt16(ip + 1);
                    uint16_t varId = bytecode->getUInt16(ip + 3);
                    push(vars[contextId].top()[varId]);
                    break;
                }
                case BC_LOADCTXSVAR: {
                    uint16_t contextId = bytecode->getUInt16(ip + 1);
                    uint16_t varId = bytecode->getUInt16(ip + 3);
                    push(vars[contextId].top()[varId]);
                    break;
                }
                case BC_STORECTXDVAR: {
                    uint16_t contextId = bytecode->getUInt16(ip + 1);
                    uint16_t varId = bytecode->getUInt16(ip + 3);
                    double value = top<double>();
                    pop();
                    vars[contextId].top()[varId] = value;
                    break;
                }
                case BC_STORECTXIVAR: {
                    uint16_t contextId = bytecode->getUInt16(ip + 1);
                    uint16_t varId = bytecode->getUInt16(ip + 3);
                    int64_t value = top<int64_t>();
                    pop();
                    vars[contextId].top()[varId] = value;
                    break;
                }
                case BC_STORECTXSVAR: {
                    uint16_t contextId = bytecode->getUInt16(ip + 1);
                    uint16_t varId = bytecode->getUInt16(ip + 3);
                    uint16_t value = top<uint16_t>();
                    pop();
                    vars[contextId].top()[varId] = value;
                    break;
                }
                case BC_DCMP: {
                    double left = top<double>();
                    pop();
                    double right = top<double>();
                    pop();
                    push(left < right ? -1 : left == right ? 0 : 1);
                    break;
                }
                case BC_ICMP: {
                    int64_t left = top<int64_t>();
                    pop();
                    int64_t right = top<int64_t>();
                    pop();
                    push(left < right ? -1 : left == right ? 0 : 1);
                    break;
                }
                case BC_JA: {
                    uint16_t offset = bytecode->getUInt16(ip + 1);
                    ip += offset + 1;
                    continue;
                    break;
                }
                case BC_IFICMPNE: {
                    uint16_t offset = bytecode->getUInt16(ip + 1);
                    int64_t left = top<int64_t>();
                    pop();
                    int64_t right = top<int64_t>();
                    pop();
                    if (left != right) {
                        ip += offset + 1;
                        continue;
                    }
                    break;
                }
                case BC_IFICMPE: {
                    uint16_t offset = bytecode->getUInt16(ip + 1);
                    int64_t left = top<int64_t>();
                    pop();
                    int64_t right = top<int64_t>();
                    pop();
                    if (left == right) {
                        ip += offset + 1;
                        continue;
                    }
                    break;
                }
                case BC_IFICMPG: {
                    uint16_t offset = bytecode->getUInt16(ip + 1);
                    int64_t left = top<int64_t>();
                    pop();
                    int64_t right = top<int64_t>();
                    pop();
                    if (left > right) {
                        ip += offset + 1;
                        continue;
                    }
                    break;
                }
                case BC_IFICMPGE: {
                    uint16_t offset = bytecode->getUInt16(ip + 1);
                    int64_t left = top<int64_t>();
                    pop();
                    int64_t right = top<int64_t>();
                    pop();
                    if (left >= right) {
                        ip += offset + 1;
                        continue;
                    }
                    break;
                }
                case BC_IFICMPL: {
                    uint16_t offset = bytecode->getUInt16(ip + 1);
                    int64_t left = top<int64_t>();
                    pop();
                    int64_t right = top<int64_t>();
                    pop();
                    if (left < right) {
                        ip += offset + 1;
                        continue;
                    }
                    break;
                }
                case BC_IFICMPLE: {
                    uint16_t offset = bytecode->getUInt16(ip + 1);
                    int64_t left = top<int64_t>();
                    pop();
                    int64_t right = top<int64_t>();
                    pop();
                    if (left <= right) {
                        ip += offset + 1;
                        continue;
                    }
                    break;
                }
                case BC_STOP: {
                    break;
                }
                case BC_CALL: {
                    uint16_t calledFunctionId = bytecode->getUInt16(ip + 1);
                    BytecodeFunction *calledFunction = dynamic_cast<BytecodeFunction *>(this->functionById(calledFunctionId));
                    vars[calledFunctionId].push(vector<uint64_t>(calledFunction->localsNumber()));
                    executeFunction(calledFunction);
                    break;
                }
                case BC_CALLNATIVE: {
                    assert(0);
                    break;
                }
                case BC_RETURN: {
                    vars[functionId].pop();
                    return;
                    break;
                }
                default: {
                    cout << "UNKNOWN: "  << current << endl;
                    assert(0);
                }
            }
            ip += instrSize[current];
        }
    }
};
}

#endif