#pragma once

#include "ast.h"
#include "mathvm.h"
#include <stack>
#include <vector>
#include <stdint.h>


struct InterpretationException {
  InterpretationException(std::string const& message) : myMessage(message) {
  }
  virtual std::string what() const {
    return myMessage;
  }
private:
  std::string myMessage;
};

struct StackVariable {
  union {
    double d;
    int64_t i;
    char const * s;
  };
};

struct StackFrame {
  std::vector<StackVariable> vars;
  uint32_t ip;
  StackFrame * prevFrame;
  uint16_t functionId;
  StackFrame(uint16_t variablesNum, uint16_t functionId) : functionId(functionId), ip(0), prevFrame(NULL) {
    vars.resize(variablesNum);
  }
};

struct Interpeter : mathvm::Code {
  virtual mathvm::Status* execute(std::vector<mathvm::Var*>& vars);
  Interpeter();

private:

  std::stack<StackFrame*> myFrameStack;
  std::stack<StackVariable> myStack;
  uint32_t* myIP;
  mathvm::Bytecode * myCurrentBytecode;
  StackFrame * myCurrentFrame;

  StackFrame* AllocateFrame(uint16_t functionId);
  void DeallocateFrame();
  
  template<typename T>
  T Next() {
    T result = myCurrentBytecode->getTyped<T>(*myIP);
    *myIP += sizeof(T);
    return result;
  }

  mathvm::Instruction GetNextInstruction();

  void Push(int64_t value);
  void Push(double value);
  void Push(StackVariable const & var);
  int64_t PopInt();
  double PopDouble();
  void PushString( uint16_t id );
  char const * PopString();
  void PopCurrentFrame();
  void Jump( int16_t offset );
  StackFrame* FindFrame( uint16_t frameId );
  
  // Print functions, just in case if i need to output in specific format
  void Print( int64_t value );
  void Print( double value );
  void Print( char const * value );

};
