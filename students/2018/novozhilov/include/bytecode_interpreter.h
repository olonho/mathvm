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
0: ILOAD 1
9: STOREIVAR @0
12: DLOAD 2.2
21: STOREDVAR @1
24: ILOAD0
25: LOADIVAR @0
28: I2D
29: LOADDVAR @1
32: DCMP
33: IFICMPL 40
36: ILOAD0
37: JA 41
40: ILOAD1
41: IPRINT
42: STOP
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
        void compare(T upper, T lower);

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
