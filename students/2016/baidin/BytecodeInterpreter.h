#pragma once

#include "mathvm.h"
#include "stack"

namespace mathvm {
    using namespace std;

    typedef union {
        int64_t intValue;
        double doubleValue;
        uint16_t stringIdValue;
    } generalValue;

    class InterpreterException : public runtime_error {
    public:
        InterpreterException(const string &__arg);
    };

    class BytecodeInterpreter : public Code {
        vector<generalValue> locals;
        vector<uint16_t> offsets;
        vector<uint8_t> _stack;
        stack<pair<uint16_t, uint16_t>> callStack;

        ostream &out;

        uint16_t currentFunctionId;
        uint16_t localsOffset;
        uint16_t index;
        Bytecode *bytecode;
        uint16_t localsCount;

        generalValue var1;
        generalValue var2;
        generalValue var3;
        generalValue var4;

        void executeInstruction();

        void callFunction();

        void returnFromFunction();

        template<class T>
        T popTypedFromStack();

        template<class T>
        void pushTypedToStack(T value);

        int64_t getLocalInt(uint16_t index);

        int64_t getContextInt(uint16_t contextId, uint16_t index);

        void putLocalInt(uint16_t index, int64_t value);

        void putContextInt(uint16_t contextId, uint16_t index, int64_t value);

        double getLocalDouble(uint16_t index);

        double getContextDouble(uint16_t contextId, uint16_t index);

        void putLocalDouble(uint16_t index, double value);

        void putContextDouble(uint16_t contextId, uint16_t index, double value);

        uint16_t getLocalStringId(uint16_t index);

        uint16_t getContextStringId(uint16_t contextId, uint16_t index);

        void putLocalStringId(uint16_t index, uint16_t value);

        void putContextStringId(uint16_t contextId, uint16_t index, uint16_t value);

    public:
        BytecodeInterpreter(ostream &out);

        virtual Status *execute(vector<Var *> &vars);
    };
}
