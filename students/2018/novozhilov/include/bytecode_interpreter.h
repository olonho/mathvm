//
// Created by jetbrains on 18.10.18.
//

#ifndef VIRTUAL_MACHINES_INTERPRETER_CODE_H
#define VIRTUAL_MACHINES_INTERPRETER_CODE_H

#include <stack>
#include <variant>
#include <mathvm.h>
#include "interpreter_context.h"

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
        InterpreterContext *_context;
        vector<vector<InterpreterContext*>> _contextsByFunction;
        vector<BytecodeFunction*> _callStack;
        vector<uint32_t> _offsetStack;

    public:
        explicit BytecodeInterpreter(Code *code, ostream &out = cout);

        Status *executeProgram(vector<Var *> &vars);

    private:
        void execute(BytecodeFunction* mainFunction);

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

        void dumpStackTop();
    };
}

#endif //VIRTUAL_MACHINES_INTERPRETER_CODE_H
