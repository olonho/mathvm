#pragma once
#include <mathvm.h>

typedef enum { 
  //Begin of code to call func. Code includes pushing of func parameters, closures and locals on stack
  BCA_FCALL_BEGIN = static_cast<int>(mathvm::BC_LAST) + 1,
  BCA_FCALL_END,
  //Now there is function param value on tos
  BCA_FPARAM_COMPUTED,
  //Instruction specific for interpreter's calling convention
  BCA_VM_SPECIFIC,
  //Next STOREVAR instruction tells to save val on TOS to closure variable
  BCA_FPARAM_CLOSURE_SAVE,
  //Next instructions cleans up parameter value from TOS
  BCA_FPARAM_CLEANUP,
  //Esoteric things I don't know how to explain them but 
  //they allow to solve exponential problem of 
  //generating register machine code from stack machine code in linear time :)
  BCA_LOGICAL_OP_RES,
  BCA_LOGICAL_LAZY_RES,
  BCA_LOGICAL_OP_RES_END

} BytecodeAnnotations;
