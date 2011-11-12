#pragma once
#include <mathvm.h>

typedef enum { 
  //Begin of code to call func. Code includes pushing of func parameters, closures and locals on stack
  BCA_FCALL_BEGIN = static_cast<int>(mathvm::BC_LAST),
  BCA_FCALL_END,
  BCA_FPARAM_COMPUTED,
  BCA_VM_SPECIFIC,
  BCA_FPARAM_CLOSURE_SAVE,
  BCA_FPARAM_CLEANUP,
  BCA_LOGICAL_OP_RES,
  BCA_LOGICAL_OP_RES_END
} BytecodeAnnotations;
