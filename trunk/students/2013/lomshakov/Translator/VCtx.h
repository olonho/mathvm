//
// Created by Vadim Lomshakov on 16/12/13.
// Copyright (c) 2013 spbau. All rights reserved.
//

#ifndef __VCtx_H_
#define __VCtx_H_

#include <stdint.h>
#include <stack>

namespace mathvm {
#define MAX_STACK_SIZE 500*1000
#define MAX_DISPLAY_SIZE 60   //  max depth into activation tree



// compiler doesn't support accessing to call stack
// see: https://code.google.com/p/asmjit/issues/detail?id=87
// and I feel not confident with assembler, because we use virtual ctx as work around
// virtual context
struct VCtx {
  int64_t BP;
  int64_t SP;

  char stack[MAX_STACK_SIZE];

  int64_t display[MAX_DISPLAY_SIZE];

  static const uint32_t VBP_OFFSET = 0;
  static const uint32_t PREV_LINK_ACCESS_OFFSET = 1;
  static const uint32_t ARGS_BEGIN_OFFSET = 2;

  static const uint16_t DISPLAY_REC_SIZE = 8; // size
  VCtx() : BP(0), SP(0) {}
};

}

#endif //__VCtx_H_
