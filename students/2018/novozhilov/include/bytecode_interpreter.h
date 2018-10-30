//
// Created by jetbrains on 18.10.18.
//

#ifndef VIRTUAL_MACHINES_INTERPRETER_CODE_H
#define VIRTUAL_MACHINES_INTERPRETER_CODE_H

#include <stack>
#include <variant>
#include <mathvm.h>
#include "context.h"

/*
function <top>
0: ILOAD 1
9: STOREIVAR @1
12: DLOAD 2
21: STOREDVAR @0
24: SLOAD @1
27: SPRINT
28: LOADIVAR @1
31: ILOAD 1
40: IADD
41: IPRINT
42: SLOAD @2
45: SPRINT
46: SLOAD @3
49: SPRINT
50: LOADDVAR @0
53: DLOAD 1
62: DADD
63: DPRINT
64: SLOAD @2
67: SPRINT
68: SLOAD @4
71: SPRINT
72: LOADIVAR @1
75: ILOAD 123456789012345
84: IADD
85: IPRINT
86: SLOAD @2
89: SPRINT
90: SLOAD @5
93: SPRINT
94: LOADDVAR @0
97: DLOAD 1e+09
106: DADD
107: DPRINT
108: SLOAD @2
111: SPRINT
112: STOP
*/

namespace mathvm {
    class InterpreterCodeImpl : public Code {
    public:
        Status *execute(vector<Var*> &vars) override;
    };

    class BytecodeInterpreter {
    private:
        Code *_code;
        ostream &_out;
        stack<StackValue > _stack;
        Context *_context;

    public:
        explicit BytecodeInterpreter(Code *code, ostream &out = cout);

        Status *execute(vector<Var*> &vars);

        void executeFunction(BytecodeFunction *function);

    private:
        BytecodeFunction *getFunctionByName(string name);

        void enterContext(BytecodeFunction *function);
        void exitContext();

        void loadVar(Bytecode* bytecode, Instruction instruction, uint32_t &i);
        void storeVar(Bytecode* bytecode, Instruction instruction, uint32_t &i);

        void jump(Bytecode* bytecode, uint32_t &i);
        void conditionalJump(Bytecode* bytecode, Instruction instruction, uint32_t &i);

        template <class T>
        T getTyped(Bytecode *bytecode, uint32_t &i);
        Instruction getInstruction(Bytecode *bytecode, uint32_t &i);
        double getDouble(Bytecode *bytecode, uint32_t &i);
        int16_t getInt16(Bytecode *bytecode, uint32_t &i);
        uint16_t getUInt16(Bytecode *bytecode, uint32_t &i);
        int32_t getInt32(Bytecode *bytecode, uint32_t &i);
        int64_t getInt64(Bytecode *bytecode, uint32_t &i);

        void pushInt(int64_t value);
        void pushDouble(double value);
        void pushString(string value);

        template<class T> T popTyped();
        int64_t popInt();
        double popDouble();

        string popString();
    };
}

#endif //VIRTUAL_MACHINES_INTERPRETER_CODE_H
