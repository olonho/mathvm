#pragma once

#include "ast.h"
#include "mathvm.h"
#include <stack>
#include <vector>
#include <stdint.h>
#include <cstdarg>
#include <stdio.h>


struct InterpretationException {
  InterpretationException(std::string const& message) : myMessage(message) {
  }
  virtual std::string what() const {
    return myMessage;
  }
  InterpretationException(mathvm::AstNode * where, std::string const& message) : myMessage(message), myNode(where) {
  }

  InterpretationException(mathvm::AstNode * where, char* message, ...) : myNode(where){
    char buf[512];
    va_list argptr; 
    va_start(argptr, message);
    vsprintf(buf, message, argptr);
    va_end(argptr); 
    myMessage = buf;
  }
private:
  std::string myMessage;
  mathvm::AstNode* myNode;
};

#pragma pack(1)
struct StackVariable {
  union {
    double d;
    int64_t i;
    char const * s;
  };

  template<typename T>
  T getTyped(T const& var);
};

#pragma pack(1)
struct StackFrame {
  uint32_t ip;
  StackFrame * prevFrame;
  StackFrame * prevDifferentFrame;
  uint16_t functionId;
  size_t size;
  StackVariable* vars;
  StackFrame(uint16_t variablesNum, uint16_t functionId) : ip(0), prevFrame(NULL), prevDifferentFrame(NULL), functionId(functionId) {
    size = sizeof(StackFrame) + variablesNum * sizeof(StackVariable);
    char* p = (char*)&vars;
    p += sizeof(vars);
    vars = (StackVariable*)p;
  }

};

struct Interpeter : mathvm::Code {
  virtual mathvm::Status* execute(std::vector<mathvm::Var*>& vars);
  Interpeter();

private:

  uint32_t* myIP;
  mathvm::Bytecode * myCurrentBytecode;
  StackFrame * myCurrentFrame;
  int myFrameStackPoolSize;
  char* myFrameStackPool;
  int myFrameStackPoolIP;
  StackVariable * myVariablesStack;
  int myVariablesStackIP;
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
  void AllocateFrameStack( int stackSizeInKb );
  StackFrame * AddFrame( uint16_t localsNumber, uint16_t functionId );
};
