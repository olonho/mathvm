#pragma once

#include <cstdio>
#include <cstdlib>

#include "mathvm.h"

// ================================================================================

#define VISIT(type)                             \
  void visit##type(type* node)


#define ABORT(x...)                             \
  printf(x);                                    \
  abort();

// --------------------------------------------------------------------------------

namespace mathvm {
  class NativeFunction;

  enum ValType {
    VAL_INVALID = 0,
    VAL_VOID,
    VAL_DOUBLE,
    VAL_INT,
    VAL_STRING,
    VAL_BOOL
  };

  struct VarInfo {
    enum Kind {
      KV_LOCAL,
      KV_ARG
    };
    
    Kind kind;
    int fPos;
    NativeFunction* owner;
  };

  struct NodeInfo {
    ValType type;
    NativeFunction* funRef;
    const char* string;
  };
}

#define VAR_INFO(v) ((VarInfo*)(v->info()))
#define NODE_INFO(n) ((NodeInfo*)(n->info()))

