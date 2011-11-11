#pragma once
#include <mathvm.h>

typedef enum { 
  //Begin of code to call func. Code includes pushing of func parameters, closures and locals on stack
  BE_FCALL_BEGIN = static_cast<int>(mathvm::BC_LAST),
  BE_FCALL_END
} BytecodeExts;
