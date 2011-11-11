#pragma once
#include <mathvm.h>

typedef enum { 
  //Begin of code to call func. Code includes pushing of func parameters, closures and locals on stack
  BCE_FCALL_BEGIN = static_cast<int>(mathvm::BC_LAST),
  BCE_FCALL_END
} BytecodeExts;
