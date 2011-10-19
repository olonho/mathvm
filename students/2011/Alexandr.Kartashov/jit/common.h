#include <cstdio>
#include <cstdlib>

// ================================================================================

#define VISIT(type)                             \
  void visit##type(type* node)


#define ABORT(x...)                             \
  printf(x);                                    \
  abort();

