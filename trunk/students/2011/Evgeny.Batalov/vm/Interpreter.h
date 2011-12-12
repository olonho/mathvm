#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include "ByteCodeAnnotations.h"
#include "Executable.h"

#define STACK_SIZE 32 * 1024 * 1024
#define CALL_STACK_SIZE 4 * 1024 * 1024

typedef union {
  size_t any_;
  double double_;
  int64_t int_;
  const char* str_;
} UserStackItem;

struct CallStackItem {
  uint16_t func_id;
  ByteCodeElem ip;
  UserStackItem* frame_beg;
};

typedef size_t (*InterpreterNativeFunc)(void* sp);
typedef std::map<uint16_t, InterpreterNativeFunc> InterpreterNativeFuncMap;

class Interpreter {

  Executable& executable;
  CallStackItem* callStack;
  UserStackItem *userStack;
  size_t callStackPos;
  size_t userStackPos;
  VMRegisters regs;
  InterpreterNativeFuncMap nativeFuncMap;
  
  InterpreterNativeFunc getNativeFunc(uint16_t id);

  public:
  Interpreter(Executable& executable) 
    : executable(executable) 
      , callStackPos(0)
      , userStackPos(0)
  {
    userStack = (UserStackItem*)malloc(STACK_SIZE);
    callStack = (CallStackItem*)malloc(CALL_STACK_SIZE);
    //memset(userStack, 0, STACK_SIZE);
    //memset(callStack, 0, CALL_STACK_SIZE);
  }

  ~Interpreter() {
    free(userStack);
    free(callStack);
  }

  void interpret();
};
