#pragma once

#include <cstdio>
#include <cstdlib>

// ================================================================================

#define VISIT(type)                             \
  void visit##type(type* node)


#define ABORT(x...)                             \
  printf(x);                                    \
  abort();

// --------------------------------------------------------------------------------

namespace mathvm {
  class NativeFunction;

  struct VarInfo {
    enum Kind {
      KV_LOCAL,
      KV_ARG
    };
    
    Kind kind;
    int fPos;
    NativeFunction* owner;
  };
}

#define VAR_INFO(v) ((VarInfo*)(v->info()))

