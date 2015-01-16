#pragma once

#include "common.h"
#include "my_translator.h"

using namespace mathvm;

class StackUnit {
private:
    union {
        double doubleVal;
        int64_t intVal;
        char const * stringVal;
    };
public:
    StackUnit() {}

    StackUnit(double val) {
        doubleVal = val;
    }
    StackUnit(int64_t val) {
        intVal = val;
    }
    StackUnit(char const * val) {
        stringVal = val;
    }

    template<class T>
    T getValue() const {
        return T::inexistent_function;
    }
    double getDouble() {
        return doubleVal;
    }
    int64_t getInt() {
        return intVal;
    }
    char const * getString() {
        return stringVal;
    }
    void setDouble(double val) {
        doubleVal = val;
    }
    void setInt(int64_t val) {
        intVal = val;
    }
    void setString(char const * val) {
        stringVal = val;
    }

};

class BytecodeContext {
public:
    BytecodeContext(): ip(0), id(0), vars(0), byteCode(0) {}

    BytecodeContext(BytecodeFunction * function)
            : ip(0), id(function->id()), vars(function->localsNumber()), byteCode(function->bytecode()) {}
    void setVar(idType varId, int64_t varValue);
    void setVar(idType varId, double varValue);
    void setVar(idType varId, char const * varValue);
    int64_t getIntVar(idType varId);
    double getDoubleVar(idType varId);
    char const * getStringVar(idType varId);
    int64_t nextInt();
    double nextDouble();
    idType nextId();
    offsetType nextOffset();
    void jump(offsetType offset);
    template<class T>
    T getValue(idType id) const {
        return vars[id].getValue<T>();
    }
    template<class T>
    T nextValue() {
        T value = byteCode->getTyped<T>(ip);
        ip += sizeof(T);
        return value;
    }
    idType getId();
    bool ipIsValid();
    Instruction getInstruction();
private:
    uint32_t ip;
    idType id;
    std::vector<StackUnit> vars;
    Bytecode* byteCode;
};

class BytecodeInterpreter {
public:
    BytecodeInterpreter(Code* argCode, ostream& stream);
    Status* interpret();

private:
    Code* code;
    Status* status;
    ostream& outStream;
    bool canInterpret();
    Instruction nextInstruction();
    //BytecodeContext* currentContext();
    void interpretInstructions();
    void pushOnStack(StackUnit v);
    StackUnit popFromStack();
    BytecodeContext& currentContext();
    BytecodeContext& contextById(idType id);
    template<class T>
    void storeContextVar() {
        BytecodeContext& outer = contextById(currentContext().nextId());
        outer.setVar(currentContext().nextId(), popFromStack().getValue<T>());
    }
    template<class T>
    void loadContextVar() {
        BytecodeContext& outer = contextById(currentContext().nextId());
        pushOnStack(outer.getValue<T>(currentContext().nextId()));
    }
    template<class T>
    int64_t compare(bool pushResOnStack) {
        T top = popFromStack().getValue<T>();
        T bot = popFromStack().getValue<T>();
        int64_t res = 0;
        if (bot < top) {
            res = -1;
        } else if (top < bot) {
            res = 1;
        }
        if (pushResOnStack) {
            pushOnStack(res);
        }
        return res;
    }
    void jump();
    void jumpCond(bool cond);
    void call(idType funId);
    void _return();

    std::vector<StackUnit> stack;
    std::vector<BytecodeContext> contexts;
};