#include "Interpreter.h"

void Interpreter::interpret() {
  using namespace mathvm;
  MyBytecodeFunction* main = executable.getMain(); 
  CallStackItem *cstack = (CallStackItem*)callStack;
  UserStackItem *stack  = (UserStackItem*)userStack;
  cstack->func_id = executable.idByFunc(main);
  cstack->ip.instr_ = main->bytecode()->raw();
  cstack->frame_beg = stack;
  UserStackItem tmp;

  //Invariants: stack always points to free space (+1 from really used stack)
  //            ip points to next byte after current instruction
  while(1) {
    switch (*cstack->ip.instr_++) {
      case BC_DLOAD:     stack++->double_      = *cstack->ip.double_++; break;
      case BC_ILOAD:     stack++->int_         = *cstack->ip.int_++; break;
      case BC_SLOAD:     stack++->str_         = executable.sConstById(*cstack->ip.str_++).c_str(); break;
      case BC_DLOAD0:    stack++->double_      =  0; break;
      case BC_ILOAD0:    stack++->int_         =  0; break;
      case BC_SLOAD0:    stack++->str_         = ""; break;
      case BC_DLOAD1:    stack++->double_      =  1.0; break;
      case BC_ILOAD1:    stack++->int_         =  1; break;
      case BC_DLOADM1:   stack++->double_      = -1.0; break;
      case BC_ILOADM1:   stack++->int_         = -1; break;
      case BC_DNEG:      (stack - 1)->double_ *= -1.0; break;
      case BC_INEG:      (stack - 1)->int_    *= -1; break;
      case BC_DPRINT:    std::cout << (--stack)->double_; break;//printf("%lf\n", (--stack)->double_); break;
      case BC_IPRINT:    std::cout << (--stack)->int_; break;//printf("%ld\n", (--stack)->int_); break;
      case BC_SPRINT:    std::cout << (--stack)->str_; break;//printf("%s\n",  (--stack)->str_); break;
      case BC_I2D:       (stack - 1)->double_ = (stack - 1)->int_; break;
      case BC_POP:       --stack; break;
      case BC_STOP:      return;
      case BC_LOADIVAR:  (stack++)->int_ = (cstack->frame_beg + *cstack->ip.var_++)->int_; break;
      case BC_LOADDVAR:  (stack++)->double_ = (cstack->frame_beg + *cstack->ip.var_++)->double_; break;
      case BC_STOREIVAR: (cstack->frame_beg + *cstack->ip.var_++)->int_    = (--stack)->int_; break;
      case BC_STOREDVAR: (cstack->frame_beg + *cstack->ip.var_++)->double_ = (--stack)->double_; break;
      case BC_LOADIVAR0: (stack++)->int_    = regs.ir0;   break;
      case BC_LOADDVAR0: (stack++)->double_ = regs.dr0;   break;
      case BC_STOREIVAR0:regs.ir0 = (--stack)->int_;      break;
      case BC_STOREDVAR0:regs.dr0 = (--stack)->double_;   break;
      case BC_JA:        cstack->ip.instr_ += *cstack->ip.jmp_; break;
      case BC_IFICMPNE:  stack -= 2; if ((stack + 1)->int_ != stack->int_)
                         cstack->ip.instr_ += *cstack->ip.jmp_; else cstack->ip.instr_ += 2; break;
      case BC_IFICMPE :  stack -= 2; if ((stack + 1)->int_ == stack->int_)
                         cstack->ip.instr_ += *cstack->ip.jmp_; else cstack->ip.instr_ += 2; break;
      case BC_IFICMPG :  stack -= 2; if ((stack + 1)->int_ > stack->int_)
                         cstack->ip.instr_ += *cstack->ip.jmp_; else cstack->ip.instr_ += 2; break;
      case BC_IFICMPGE:  stack -= 2; if ((stack + 1)->int_ >= stack->int_)
                         cstack->ip.instr_ += *cstack->ip.jmp_; else cstack->ip.instr_ += 2; break;
      case BC_IFICMPL :  stack -= 2; if ((stack + 1)->int_ < stack->int_)
                         cstack->ip.instr_ += *cstack->ip.jmp_; else cstack->ip.instr_ += 2; break;
      case BC_IFICMPLE:  stack -= 2; if ((stack + 1)->int_ <= stack->int_)
                         cstack->ip.instr_ += *cstack->ip.jmp_; else cstack->ip.instr_ += 2; break;
      case BC_DADD:      tmp.double_ = (--stack)->double_;
                         (stack - 1)->double_ += tmp.double_;
                         break;
      case BC_IADD:      tmp.int_ = (--stack)->int_;
                         (stack - 1)->int_ += tmp.int_;
                         break;
      case BC_DSUB:      tmp.double_ = (--stack)->double_;
                         (stack - 1)->double_ = tmp.double_ - (stack - 1)->double_;
                         break;
      case BC_ISUB:      tmp.int_ = (--stack)->int_;
                         (stack - 1)->int_ = tmp.int_ - (stack - 1)->int_;
                         break;
      case BC_DMUL:      tmp.double_ = (--stack)->double_;
                         (stack - 1)->double_ *= tmp.double_;
                         break;
      case BC_IMUL:      tmp.int_ = (--stack)->int_;
                         (stack - 1)->int_ *= tmp.int_;
                         break;
      case BC_DDIV:      tmp.double_ = (--stack)->double_;
                         (stack - 1)->double_ = tmp.double_ / (stack - 1)->double_;
                         break;
      case BC_IDIV:      tmp.int_ = (--stack)->int_;
                         (stack - 1)->int_ = tmp.int_ / (stack - 1)->int_;
                         break;
      case BC_SWAP:      tmp = *(stack - 2);
                         *(stack - 2) = *(stack - 1);
                         *(stack - 1) = tmp;
                         break;
      case BC_DCMP:      tmp.double_ = (--stack)->double_;
                         (stack - 1)->int_ = tmp.double_ < (stack - 1)->double_ ? 1 : 
                                            (tmp.double_ == (stack - 1)->double_ ? 0 : -1);
                         break;
      case BC_CALL:      {
                         uint16_t func_id = *cstack->ip.var_++;
                         MyBytecodeFunction *func = executable.funcById(func_id);
                         CallStackItem tmp = *cstack;
                         tmp.func_id = func_id;
                         tmp.ip.instr_ = func->bytecode()->raw();
                         tmp.frame_beg = stack - func->translatableFunc()->getFrameSize();
                         ++cstack;
                         *cstack = tmp;
                         }
                         break;
      case BC_RETURN:    --cstack;
                         break;
    };
  }
}
