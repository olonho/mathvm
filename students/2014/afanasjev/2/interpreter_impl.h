#pragma once

#include "mathvm.h"
#include "native_call_builder.h"

namespace mathvm {
union Value {
    double doubleVal;
    int64_t intVal;
    const char* stringVal;
};

class InterpreterCodeImpl : public Code {
    virtual Status* execute(vector<Var*>& vars);

    void run();
    void callFunction(uint16_t funId);
    void retFunction();
    void callNative(uint16_t id);
    
    void popTwo(Value& left, Value& right);
    Value pop();
    void push(Value val);
    
    template<class T> 
    int cmp(T const & left, T const & right);

    BytecodeFunction* functionById(uint16_t index) const;
    void initNatives();
private:
    struct RetRecord {
        int funId;
        int bci;
        int lastFrame;

        RetRecord() : funId(0), bci(0), lastFrame(0) {}
        RetRecord(int funId, int bci, int lastFrame)
            : funId(funId), bci(bci), lastFrame(lastFrame)
        {}
    };

    void fail(string msg);
    vector<Value> stack;
    vector<RetRecord> callStack;

    vector<nativeCall> natives;
    
    vector<int> lastFunctionFrame;
    int currentFrame;
    Bytecode* bc;
    int currentFunId;
    size_t bci;
};
}
