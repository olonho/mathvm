#ifndef BYTECODEINTERPRETER_HPP
#define BYTECODEINTERPRETER_HPP

#include <vector>
#include <map>

using std::vector;
using std::map;

#include "mathvm.h"

using namespace mathvm;
typedef union _v {
    double _d;
    int64_t _i;
    uint16_t _ui;
    _v() {}
    _v(double d) : _d(d) {}
    _v(int64_t i) : _i(i) {}
    _v(uint16_t ui): _ui(ui) {}
//    operator double () const {return _d;}
//    operator int64_t () const {return _i;}
//    operator uint64_t () const {return _ui;}
} Value;

class BytecodeInterpreter : public Code {

public:
    struct StackFrame {
        StackFrame *parent;
        uint32_t v_offset;
        uint32_t o_offset;
        uint32_t pointer;
        BytecodeFunction *function;
    };

    BytecodeInterpreter() {}
    ~BytecodeInterpreter() {}

    virtual Status *execute(vector<Var *> &);
    //    void processInstruction(Instruction instruction);
    void processBinaryOperation(Instruction instruction);
    void processUnaryOperaion(Instruction instruction);
    void processPrint(Instruction instruction);
    void processCall(uint16_t functionId);
    void processReturn();
    void processConditionalJump(Instruction instruction);
    uint32_t findVarIndex (uint16_t ctxID, uint16_t varID);

private:

    vector<Value> variableStack;
    vector<Value> operandStack;
    StackFrame *frame;
    Bytecode *bc;
    uint32_t pointer;
    map<uint16_t,map<uint16_t,uint32_t>> cache;
};

#endif // BYTECODEINTERPRETER_HPP
