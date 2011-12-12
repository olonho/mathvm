#include <utility>
#include <AsmJit/Compiler.h>
#include <AsmJit/Logger.h>
#include <AsmJit/MemoryManager.h>
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
  size_t callCount = 0;
  //Invariants: stack always points to free space (+1 from really used stack)
  //            ip points to next byte after current instruction
  while(true) {
    switch (*cstack->ip.instr_++) {
      case BC_DLOAD:     stack++->double_      = *cstack->ip.double_++; break;
      case BC_ILOAD:     stack++->int_         = *cstack->ip.int_++; break;
      case BC_SLOAD:     stack++->str_         = executable.chConstById(*cstack->ip.str_++); break;
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
      case BC_IPRINT:    std::cout << (--stack)->int_; break;   //printf("%ld\n", (--stack)->int_); break;
      case BC_SPRINT:    std::cout << (--stack)->str_; break;   //printf("%s\n",  (--stack)->str_); break;
      case BC_I2D:       (stack - 1)->double_ = (stack - 1)->int_; break;
      case BC_D2I:       (stack - 1)->int_ = (stack - 1)->double_; break;
      case BC_POP:       --stack; break;
      case BC_STOP:      return;
      case BC_LOADIVAR:  (stack++)->int_ = (cstack->frame_beg + *cstack->ip.var_++)->int_; break;
      case BC_LOADDVAR:  (stack++)->double_ = (cstack->frame_beg + *cstack->ip.var_++)->double_; break;
      case BC_LOADSVAR:  (stack++)->str_ = (cstack->frame_beg + *cstack->ip.var_++)->str_; break;
      case BC_STOREIVAR: (cstack->frame_beg + *cstack->ip.var_++)->int_    = (--stack)->int_; break;
      case BC_STOREDVAR: (cstack->frame_beg + *cstack->ip.var_++)->double_ = (--stack)->double_; break;
      case BC_STORESVAR: (cstack->frame_beg + *cstack->ip.var_++)->str_ = (--stack)->str_; break;
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
      
      case BC_IMOD:      tmp.int_ = (--stack)->int_;
                         (stack - 1)->int_ = tmp.int_ % (stack - 1)->int_;
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
                         (stack - 1)->int_ = tmp.double_ > (stack - 1)->double_ ? 1 : 
                                            (tmp.double_ == (stack - 1)->double_ ? 0 : -1);
                         break;
      case BC_CALL:      {++callCount;
                         uint16_t func_id = *cstack->ip.var_++;
                         MyBytecodeFunction *func = executable.funcById(func_id);
                         CallStackItem tmp = *cstack;
                         tmp.func_id = func_id;
                         tmp.ip.instr_ = func->bytecode()->raw();
                         tmp.frame_beg = stack - func->translatableFunc()->getFrameSize();
                         ++cstack;
                         *cstack = tmp;}
                         break;
      case BC_CALLNATIVE:{ uint16_t func_id = *cstack->ip.var_++;
                           InterpreterNativeFunc interp_func = 
                             getNativeFunc(func_id);
                           MyNativeFunction *native_func = 
                             executable.nativeFuncById(func_id);
                           size_t res = interp_func((void*)(stack - 1));
                           //stack -= native_func->getSignature().size() - 1;
                           if (native_func->getReturnType() != VT_VOID) {
                             stack++->any_ = res;
                           }
                         } break;
      case BC_RETURN:    --cstack;
                         break;
      case BCA_FPARAM_COMPUTED: ++cstack->ip.byte_;  break;
      default: break;
    };
  }
}

struct Param {
  mathvm::VarType type;
  AsmJit::BaseVar* var;
};

InterpreterNativeFunc Interpreter::getNativeFunc(uint16_t id) {
  using namespace mathvm;
  using namespace AsmJit;
  InterpreterNativeFuncMap::iterator it =  
    nativeFuncMap.find(id);
  if (it != nativeFuncMap.end()) {
    return it->second;
  } else {
    InterpreterNativeFunc res = NULL;
    Compiler cc;
    MyNativeFunction *native_func = 
      executable.nativeFuncById(id);
    const size_t sp_arg = 0;
#ifdef VERBOSE
    FileLogger logger(stderr);
    cc.setLogger(&logger);
#endif
    cc.newFunction(CALL_CONV_DEFAULT, FunctionBuilder1<size_t, void*>()); //ret sp
    cc.getFunction()->setHint(FUNCTION_HINT_NAKED, true);
    //create native func arg variabless;
    std::vector<Param> params;
    Signature::const_reverse_iterator sit, sit_end;
    sit = native_func->getSignature().rbegin();
    sit_end = native_func->getSignature().rend();
    
    for(; sit != sit_end; ++sit) {
      Param param;
      param.type = sit->first;
      switch(param.type) {
        case VT_DOUBLE:{
                         XMMVar *newXMM = new XMMVar(cc.newXMM(VARIABLE_TYPE_XMM_1D));
                         param.var = newXMM;
                         break;}
        default:{
                  GPVar *newGP = new GPVar (cc.newGP());
                  param.var = newGP;
                  break;}
      }
      params.push_back(param);
    }
    params.pop_back(); //last is return type

    //copy mem values from sp to new vars
    //and create prototype for native function
    FunctionBuilderX fBuilder;
    for(size_t i = 0; i != params.size(); ++i) {
      switch(params[i].type) {
        case VT_DOUBLE:
          cc.movq(*(XMMVar*)params[i].var,
              ptr(cc.argGP(sp_arg), -sizeof(uint64_t) * (params.size() - 1 - i)));
          fBuilder.addArgument<double>();
        break;
        default:
          cc.mov(*(GPVar*)params[i].var,
              ptr(cc.argGP(sp_arg), -sizeof(uint64_t) * (params.size() - 1 - i)));
          fBuilder.addArgument<int64_t>();
          break;
      }
    }
    
    //set return value type in prototype
    switch(native_func->getReturnType()) {
        case VT_DOUBLE:
          fBuilder.setReturnValue<double>();
        break;
        case VT_VOID:
          fBuilder.setReturnValue<void>();
        break;
        default:
          fBuilder.setReturnValue<int64_t>();
        break;
    }
    //generate call to prototyped function
    ECall* _call = cc.call(imm((size_t)native_func->getPtr()));
    _call->setPrototype(CALL_CONV_DEFAULT, fBuilder);
    for(size_t i = 0; i != params.size(); ++i) {
      switch(params[i].type) {
        case VT_DOUBLE:
          _call->setArgument(i, *(XMMVar*)params[i].var);
        break;
        default:
          _call->setArgument(i, *(GPVar*)params[i].var);
        break;
      }
    }

    switch(native_func->getReturnType()) {
      case VT_DOUBLE:{
        GPVar retGP = cc.newGP();
        XMMVar retXMM = cc.newXMM();
        _call->setReturn(retXMM);
        cc.movq(retGP, retXMM);
        cc.ret(retGP);
        break;}
      case VT_VOID:{
         GPVar retGP = cc.newGP();
         cc.mov(retGP, imm(0));
         cc.ret(retGP); 
        break;}
      default:{
        GPVar retGP = cc.newGP();
        _call->setReturn(retGP);
        cc.ret(retGP);
        break;}
    }
    //epilog
    cc.endFunction();
    res = (InterpreterNativeFunc)cc.make();
    nativeFuncMap[id] = res;
    return res;
  }
}
