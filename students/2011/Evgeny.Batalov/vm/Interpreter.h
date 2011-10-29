#pragma once
#include <stdlib.h>
#include <stdio.h>
#include "Executable.h"

#define STACK_SIZE 32 * 1024 * 1024
#define CALL_STACK_SIZE 32 * 1024 * 1024

typedef union {
  double double_;
  int64_t int_;
  const char* str_;
} UserStackItem;

typedef union {
  double   *double_;
  int64_t  *int_;
  uint16_t *str_;
  uint8_t  *instr_;
  int16_t  *jmp_;
  uint16_t *var_;
} ByteCodeElem;

struct CallStackItem {
  uint16_t func_id;
  ByteCodeElem ip;
  UserStackItem* frame_beg;
};

struct Registers {
  uint64_t ir0;
  uint64_t ir1;
  uint64_t ir2;
  uint64_t ir3;
  double dr0;
  double dr1;
  double dr2;
  double dr3;
};

class Interpreter {

  Executable& executable;
  CallStackItem* callStack;
  UserStackItem *userStack;
  size_t callStackPos;
  size_t userStackPos;
  Registers regs;

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
