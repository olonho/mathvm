#ifndef BYTECODE_INTERPRETER_H
#define BYTECODE_INTERPRETER_H

#include "mathvm.h"
#include "my_utils.h"

using namespace mathvm;

class BytecodeInterpreter {
    Code* _code;

    union StackVal {
        int64_t vInt;
        double vDouble;

        StackVal(): vInt(0) {} // this one is just to use resize
        StackVal(int64_t vi): vInt(vi) {}
        StackVal(double vd): vDouble(vd) {}
    };
    typedef vector<StackVal> Context;
    vector<Context> _contexts;
    vector<StackVal> _programStack;
    ExecStatus _status;

    const string MAIN_FUN_NAME = "<top>";
    uint16_t EMP_STR_ID;

public:
    void interpret(Code* code);
    ExecStatus status() const { return _status; }

private:
    void interpFun(Bytecode* bc);

    /*
     * Executes instruction from bc at pos if possible and returns
     * the offset for the next pos
     */
    int32_t execInsn(Bytecode* bc, uint32_t pos);

    pair<StackVal, StackVal> getOperands();
    pair<StackVal, StackVal> popOperands();

    uint8_t loadvar(Bytecode* bc, uint32_t pos);
    void storeValueLocal(Bytecode* bc, uint32_t pos, StackVal val);

    uint8_t loadctxvar(Bytecode* bc, uint32_t pos);
    void storeValueGlobal(Bytecode* bc, uint32_t pos, StackVal val);

    string invalidBcMsg(Instruction insn) {
        return "invalid bytecode instruction encountered: " + string(bytecodeName(insn));
    }

    string divByZeroMsg(string const& funName) {
        return "division by zero in func: " + funName;
    }
};

#endif // INTERPRETER_H
