//
// Created by dsavvinov on 13.11.16.
//

#ifndef MATHVM_INTERPRETER_H
#define MATHVM_INTERPRETER_H

#include <mathvm.h>
#include <stack>
#include <unordered_map>
#include "BytecodeCode.h"
#include "TranslationException.h"

namespace mathvm {

class Data {
public:
    int64_t * intValue = nullptr;
    double * doubleValue = nullptr;
    string * stringValue = nullptr;

    Data () {}

    Data(int64_t v) {
        intValue = new int64_t(v);
    }

    Data(double v) {
        doubleValue = new double(v);
    }

    Data(string & v) {
        stringValue = new string(v);
    }

    Data(const char * v) {
        stringValue = new string(v);
    }

    double getDouble() {
        if (doubleValue == nullptr) {
            throw ExecutionException("Type mismatch: expected double");
        }
        return *doubleValue;
    }

    int64_t getInt() {
        if (intValue == nullptr) {
            throw ExecutionException("Type mismatch: expected int");
        }
        return *intValue;
    }

    string * getString() {
        if (stringValue == nullptr) {
            throw ExecutionException("Type mismatch: expected string");
        }
        return stringValue;
    }

    Data(Data const & other) {
        if (other.intValue != nullptr) {
            this->intValue = new int64_t(*other.intValue);
        }

        if (other.doubleValue != nullptr) {
            this->doubleValue = new double(*other.doubleValue);
        }

        if (other.stringValue != nullptr) {
            this->stringValue = new string(*other.stringValue);
        }
    }

    Data & operator=(Data const & other) {
        Data tmp(other);
        swap(intValue, tmp.intValue);
        swap(doubleValue, tmp.doubleValue);
        swap(stringValue, tmp.stringValue);
        return *this;
    }

    ~Data() {
        if (intValue != nullptr) {
            delete intValue;
        }

        if (doubleValue != nullptr) {
            delete doubleValue;
        }

        if (stringValue != nullptr) {
            delete stringValue;
        }
    }
};

class Interpreter {
    vector < vector < vector <Data> > > storage;
    BytecodeCode * code;
    vector<Var*> & vars;
    std::stack<Data> stack;
    std::stack<uint16_t> callStack;

    int dbg = 0;
public:
    Interpreter(BytecodeCode * code, vector<Var*> & vars)
            : code(code)
            , vars(vars)
    { }

    Status * executeProgram();

    void executeFunction(BytecodeFunction *pFunction);

    void pushData(Data d) {
        stack.push(d);
    }

    template <class T>
    void push(T val) {
        stack.push(Data(val));
    }

    int64_t topInt() {
        if (stack.top().intValue != nullptr) {
            return * stack.top().intValue;
        }
        throw ExecutionException("No int value on TOS");
    }

    double topDouble() {
        if (stack.top().doubleValue != nullptr) {
            return *stack.top().doubleValue;
        }

        throw ExecutionException("No double value on TOS");
    }

    string * topString() {
        if (stack.top().stringValue != nullptr) {
            return stack.top().stringValue;
        }
        throw ExecutionException("No string value on TOS");
    }

    Data topData() {
        return stack.top();
    }

    void pop() {
        stack.pop();
    }

    Data getLocalData(uint16_t varID);

    Data getData(uint16_t ctxID, uint16_t varID);

    template <class T>
    void setLocalData(T value, uint16_t varID);

    template <class T>
    void setData(T value, uint16_t ctxID, uint16_t varID);

    void enterFunction(BytecodeFunction *id);

    void exitFunction();

    void loadCtxVar(Instruction param, Bytecode *pBytecode);
};

}
#endif //MATHVM_INTERPRETER_H
