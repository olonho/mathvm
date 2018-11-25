//
// Created by Владислав Калинин on 17/11/2018.
//

#ifndef MATHVM_BYTECODEINTERPETER_H
#define MATHVM_BYTECODEINTERPETER_H

#include "BytecodeGenerator.h"
#include "Context.h"

namespace mathvm {
    class Context;

    class StackContext;

    union Val {
        Val() {}

        Val(int64_t i) : i(i) {}

        Val(double d) : d(d) {}

        Val(uint16_t i16) : i16(i16) {}

        double d;
        int64_t i;
        uint16_t i16;
    };

    enum ArithmeticOperation {
        OR, AND, XOR, ADD, SUB, MUL, DIV, MOD
    };

    enum CompareOperation {
        EQ, NEQ, GT, GE, LT, LE
    };

    struct CallStack {
        uint32_t offset = 0;
        Bytecode *bytecode{};
        CallStack *prev{};

        explicit CallStack(Bytecode *bytecode) : bytecode(bytecode), prev(nullptr) {};

        CallStack(Bytecode *bytecode, CallStack *prev) : bytecode(bytecode), prev(prev) {};
    };

    class BytecodeInterpeter : public Code {
        Context *globalCtx{};
        vector<Val> stack{};
        StackContext *ctx{};
        CallStack *callStack{};

    public:
        explicit BytecodeInterpeter(Bytecode *bytecode, Context *globalCtx) : globalCtx(globalCtx) {
            init(bytecode);
        };

        Status *execute(vector<Var *> &vars);

    private:
        void init(Bytecode *bytecode);

        template<class T>
        T shiftAndGetValue();

        Instruction shiftAndGetInstruction();

        void pushInt64(int64_t value);

        void pushInt16(uint16_t value);

        void pushDouble(double value);

        int64_t popInt64();

        uint16_t popInt16();

        double popDouble();

        void evalIntegerExpession(ArithmeticOperation op);

        void evalDoubleExpession(ArithmeticOperation op);

        void callFunction();

        void returnFromFunction();

        void loadValueFromUpperContext(VarType type);

        void storeValueToUpperContext(VarType type);

        void compare(VarType type);

        void evalIfExpression(CompareOperation op);
    };

    class RunTimeError : std::exception {
        const char *msg;

    public:
        explicit RunTimeError(const char *msg) : msg(msg) {}
    };
}//mathvm

#endif //MATHVM_BYTECODEINTERPETER_H
