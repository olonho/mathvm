#pragma once
#include <mathvm.h>

typedef enum {
  BE_FCALL_BEGIN = 0, //Begin of code to call func. Code includes pushing of func parameters, closures and locals on stack
  BE_FCALL_END
} BytecodeExts;
