#ifndef MYINTERPRETER_H
#define MYINTERPRETER_H

#include <sstream>
#include <stack>
#include "StackVariable.h"
#include "InterpretationException.h"
#include "StackFrame.h"
#include "ExtendedBytecodeFunction.h"

class MyInterpeter: public mathvm::Code {
private:
    std::stack<StackFrame*> myFrameStack;
    std::stack<StackVariable> myStack;
    uint32_t* myIP;
    mathvm::Bytecode * myCurrentBytecode;
    StackFrame * myCurrentFrame;

    StackFrame* allocateFrame(uint16_t functionId);
    void deallocateFrame();

    template<typename T>
    T Next() {
        T result = myCurrentBytecode->getTyped<T>(*myIP);
        *myIP += sizeof(T);
        return result;
    }

    mathvm::Instruction getNextInstruction();

    void push(int64_t value);
    void push(double value);
    void push(StackVariable const & var);
    int64_t popInt();
    double popDouble();
    void pushString(uint16_t id);
    char const * popString();
    void popCurrentFrame();
    void jump(int16_t offset);
    StackFrame* findFrame(uint16_t frameId);

    void print(int64_t value);
    void print(double value);
    void print(char const * value);
public:
    virtual mathvm::Status* execute(std::vector<mathvm::Var*>& vars);
    MyInterpeter();
};
#endif
