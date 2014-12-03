#ifndef BYTECODEINTERPRETER_HPP
#define BYTECODEINTERPRETER_HPP

#include <vector>
#include <map>

using std::vector;
using std::map;

#include "mathvm.h"

using namespace mathvm;

typedef uint64_t valueType;

class BytecodeInterpreter : public Code {

public:
    BytecodeInterpreter() {}
    ~BytecodeInterpreter() {}

    virtual Status *execute(vector<Var *> &);
    void processInstruction(Instruction instruction);
    void processArithmeticOperation(Instruction instruction);
    void setLocalsSize(uint16_t functionId, uint16_t localsSize) {
        localsLayout[functionId] = max(localsLayout[functionId], localsSize);
    }

private:
    Bytecode *bc;
    uint32_t pointer;
    vector<uint64_t> variableStack;
    vector<uint64_t> operandStack;
    map<uint16_t, uint16_t> localsLayout;
};

#endif // BYTECODEINTERPRETER_HPP
