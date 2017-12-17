#pragma once

#include <stack>
#include <map>
#include <vector>

#include "interpreter_code.h"

#include "mathvm.h"


namespace mathvm {


struct Value {
    union {
        double d;
        int64_t i;
        const char *s;
    };

    Value() = default;

    Value(int64_t i_): i(i_) {
    }

    Value(double d_): d(d_) {
    }

    Value(const char *s_): s(s_) {
    } 

};

struct Interpreter {

    using Int = int64_t;
    using TwoBytes = uint16_t;

    void assignVars(std::vector<Var*>& vars);

    void saveVars(std::vector<Var*>& vars);

    Interpreter(InterpreterCodeImpl* impl):
        _code(impl)
    {
    }

    void mainLoop(); 
    
private:
    InterpreterCodeImpl* _code;

    uint64_t ip = 0;
    BytecodeFunction* currentFunction = nullptr;
    std::stack<BytecodeFunction*> funcs;
    std::stack<uint64_t> ips;

    std::stack<Value> vals;

    std::map<uint16_t, std::stack<std::map<uint16_t, Value>>> scopeVarToVal;

    Value popValue();
    
    void enterFunction(BytecodeFunction* func);
    void leaveFunction();
    Value* getVarByTwoIds(uint16_t, uint16_t);
    Bytecode* bytecode();

    template<class T>
    T getTyped();

};

}
