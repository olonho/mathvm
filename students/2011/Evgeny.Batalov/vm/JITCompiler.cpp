#include "JITCompiler.h"
#include <iostream>
#include <map>
#include <stdlib.h>

void JITCompiler::compileAll() {
  size_t funcId = 0; 
  
  for(; funcId != executable.funcCount(); ++funcId) {
    if (executable.getMetaData()
        [funcId].getProto().type == FT_MVM) {
      compileFunc(funcId);
    }
  }
}

char* JITCompiler::copyToStringPull(const char* str) {
    char* newStr = (char*)malloc(strlen(str) + 1);
    strcpy(newStr, str);
    stringPull.push_back(newStr);
    return newStr;
}

typedef union {
  AsmJit::GPVar  *gp;
  AsmJit::XMMVar *xmm;
} AnyVar;
typedef std::vector<AnyVar> SSAStack;

inline AnyVar newSSAXMMVar(AsmJit::Compiler& cc) {
  using namespace AsmJit;
  XMMVar *newXMM = new XMMVar(cc.newXMM(VARIABLE_TYPE_XMM_1D));
  AnyVar newVar; 
  newVar.xmm = newXMM;
  return newVar;
}

inline AnyVar newSSAGPVar(AsmJit::Compiler& cc) {
  using namespace AsmJit;
  GPVar *newGP = new GPVar (cc.newGP());
  AnyVar newVar; 
  newVar.gp = newGP;
  return newVar;
}

inline AnyVar newSSAXMMVar(double value, AsmJit::Compiler& cc) {
  using namespace AsmJit;
  AnyVar newVar = newSSAXMMVar(cc); 
  GPVar tmp(cc.newGP());
  int64_t *_value = (int64_t*)&value;
  cc.mov(tmp, imm(*_value));
  cc.movq(*newVar.xmm, tmp);
  cc.unuse(tmp);
  return newVar;
}

inline AnyVar newSSAGPVar(int64_t value, AsmJit::Compiler& cc) {
  using namespace AsmJit;
  AnyVar newVar =  newSSAGPVar(cc); 
  cc.mov(*newVar.gp, imm(value));
  return newVar;
}

inline AnyVar newSSAXMMVar(AsmJit::XMMVar& src, AsmJit::Compiler& cc) {
  using namespace AsmJit;
  AnyVar newVar = newSSAXMMVar(cc); 
  cc.movq(*newVar.xmm, src);
  return newVar;
}

inline AnyVar newSSAXMMVar(AsmJit::GPVar& src, AsmJit::Compiler& cc) {
  using namespace AsmJit;
  AnyVar newVar = newSSAXMMVar(cc); 
  cc.movq(*newVar.xmm, src);
  return newVar;
}

inline AnyVar newSSAGPVar(AsmJit::GPVar& src, AsmJit::Compiler& cc) {
  using namespace AsmJit;
  AnyVar newVar = newSSAGPVar(cc); 
  cc.mov(*newVar.gp, src);
  return newVar;
}

inline AnyVar newSSAXMMVar(AsmJit::Mem& src, AsmJit::Compiler& cc) {
  using namespace AsmJit;
  AnyVar newVar = newSSAXMMVar(cc); 
  cc.movq(*newVar.xmm, src);
  return newVar;
}

inline AnyVar newSSAGPVar(AsmJit::Mem& src, AsmJit::Compiler& cc) {
  using namespace AsmJit;
  AnyVar newVar = newSSAGPVar(cc); 
  cc.mov(*newVar.gp, src);
  return newVar;
}

//functions to overcome AsmJit::compiler limitations or errors
inline void printfS(char* fmt, char* str) {
  printf(fmt, str);
}

inline void printfD(char* fmt, double val) {
  printf(fmt, val);
}

inline void printfI(char* fmt, int64_t val) {
  printf(fmt, val);
}

inline int64_t idiv(int64_t a, int64_t b) {
  return a / b;
}

inline int64_t imod(int64_t a, int64_t b) {
  return a % b;
}

inline int64_t dcmp(double a, double b) {
  if (a < b)
    return -1;
  if (a > b)
    return 1;
  return 0;
}
//end of functions

//Performs translation of stack machine code using abstract interpretation
//Each put on TOS is translated into put on abstract stack of variables
//Each operation takes 2 or 1 variable form TOS and puts new variable on TOS
//which stores result of operation
//So each mutation is performed similary to SSA way
//After translation all variables ever been on abstract stack are 
//allocated in registers or machine stack using linear algorithm which uses lifetime
//of variables
//Branch problem is overcomed with no overhead in compile time or runtime
//Annotations BCA_* allow to do it
void JITCompiler::compileFunc(size_t funcId) {
  using namespace mathvm;
  using namespace AsmJit;
  MyBytecodeFunction *bcFunc = executable.funcById(funcId); 
  TranslatableFunction& funcMetadata = executable.getMetaData()[funcId];
  ByteCodeElem bc;
  bc.instr_ = bcFunc->bytecode()->raw();
  ByteCodeElem bcStart = bc;

  Compiler cc;
  #ifdef VERBOSE
  FileLogger logger(stderr);
  cc.setLogger(&logger);
  #endif
  AnyVar retVal;
  switch(funcMetadata.getProto().typeInfo.returnType) {
    case VT_INT: case VT_STRING:
      cc.newFunction(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, void*, void*>()); //ret bp sp
      retVal = newSSAGPVar(cc);
    break;
    case VT_DOUBLE:
      cc.newFunction(CALL_CONV_DEFAULT, FunctionBuilder2<double, void*, void*>());
      retVal = newSSAXMMVar(cc);
    break;
    case VT_VOID:
      cc.newFunction(CALL_CONV_DEFAULT, FunctionBuilder2<void, void*, void*>());
    break;
    default:
    break;
  }
  cc.getFunction()->setHint(FUNCTION_HINT_NAKED, true);
  AsmJit::Label lblRet = cc.newLabel();
  std::map<ByteCodeElem, AsmJit::Label> bcAddrToLabel;
  SSAStack ssaStack;
  while(true) {
    if (funcMetadata.isBCAddressJumped(bc.byte_ - bcStart.byte_)) {
      if (bcAddrToLabel.find(bc) == bcAddrToLabel.end()) {
       bcAddrToLabel[bc] = cc.newLabel();
      } cc.bind(bcAddrToLabel[bc]);
    }
    switch (*bc.instr_++) {
      case BC_DLOAD:   ssaStack.push_back(newSSAXMMVar(*bc.double_++, cc)); break;
      case BC_ILOAD:   ssaStack.push_back(newSSAGPVar(*bc.int_++, cc));     break;   
      case BC_SLOAD:   ssaStack.push_back(newSSAGPVar(
                             (int64_t)executable.chConstById(*bc.str_++), cc)); break; 
      case BC_DLOAD0:  ssaStack.push_back(newSSAXMMVar(0, cc)); break;
      case BC_ILOAD0:  if (*bc.instr_ == BCA_LOGICAL_OP_RES_END) {
                         cc.mov(*ssaStack.back().gp, imm(0));
                         //++bc.instr_;
                       } else { ssaStack.push_back(newSSAGPVar(0, cc)); }  break;
      case BC_SLOAD0:  { const char* strEmpty = ""; 
                         ssaStack.push_back(newSSAGPVar((int64_t)strEmpty, cc)); } break;
      case BC_DLOAD1:  ssaStack.push_back(newSSAXMMVar(1, cc)); break; 
      case BC_ILOAD1:  if (*bc.instr_ == BCA_LOGICAL_OP_RES_END) {
                         cc.mov(*ssaStack.back().gp, imm(1));
                         //++bc.instr_;
                       } else { ssaStack.push_back(newSSAGPVar(1, cc)); } break;  
      case BC_DLOADM1: ssaStack.push_back(newSSAXMMVar(-1, cc));break; 
      case BC_ILOADM1: ssaStack.push_back(newSSAGPVar(-1, cc)); break;
      case BC_DNEG:    { AnyVar _res = newSSAXMMVar(0, cc);
                         AnyVar _prev = ssaStack.back();
                         ssaStack.pop_back();
                         cc.subsd(*_res.xmm, *_prev.xmm);
                         ssaStack.push_back(_res);
                         cc.unuse(*_prev.xmm); } break;
      case BC_INEG:    cc.neg(*ssaStack.back().gp); break;
      case BC_DPRINT:  { const char* fmtStr = "%g";
                         AnyVar _fmtStr = newSSAGPVar((int64_t)fmtStr, cc);
                         AnyVar _toPrint = ssaStack.back(); ssaStack.pop_back();
                         ECall* _call = cc.call(imm((size_t)printfD));
                         _call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<void, char*, double>());
                         _call->setArgument(0, *_fmtStr.gp);
                         _call->setArgument(1, *_toPrint.xmm);
                         cc.unuse(*_fmtStr.gp); cc.unuse(*_toPrint.xmm);} break;
      case BC_IPRINT:   { const char* fmtStr = "%ld";
                          AnyVar _fmtStr = newSSAGPVar((int64_t)fmtStr, cc);
                          AnyVar _toPrint = ssaStack.back(); ssaStack.pop_back();
                          ECall* _call = cc.call(imm((size_t)printfI));
                          _call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<void, char*, int64_t>());
                          _call->setArgument(0, *_fmtStr.gp);
                          _call->setArgument(1, *_toPrint.gp); 
                          cc.unuse(*_fmtStr.gp); cc.unuse(*_toPrint.gp);} break;
      case BC_SPRINT:   { const char* fmtStr = "%s";
                          AnyVar _fmtStr = newSSAGPVar((int64_t)fmtStr, cc);
                          AnyVar _toPrint = ssaStack.back(); ssaStack.pop_back();
                          ECall* _call = cc.call(imm((size_t)printfS));
                          _call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<void, char*, size_t>());
                          _call->setArgument(0, *_fmtStr.gp);
                          _call->setArgument(1, *_toPrint.gp); 
                          cc.unuse(*_fmtStr.gp); cc.unuse(*_toPrint.gp);} break;
      case BC_I2D:      {AnyVar _int = ssaStack.back();
                          ssaStack.pop_back();
                          AnyVar _double = newSSAXMMVar(cc);
                          cc.cvtsi2sd(*_double.xmm, *_int.gp);
                          ssaStack.push_back(_double);
                          cc.unuse(*_int.gp);} break; 
      case BC_D2I:       {AnyVar _int = newSSAGPVar(cc);
                           AnyVar _double = ssaStack.back();
                           ssaStack.pop_back();
                           cc.cvttsd2si(*_int.gp, *_double.xmm);
                           ssaStack.push_back(_int);
                         } break;
      case BC_POP:       { AnyVar var = ssaStack.back(); ssaStack.pop_back(); cc.unuse(*var.gp); } 
                         break; 
      case BC_STOP:     cc.bind(lblRet);
                        switch(bcFunc->returnType()) {
                          case VT_INT: case VT_STRING:
                            { cc.ret(*retVal.gp); } break;
                          case VT_DOUBLE: 
                            { cc.ret(*retVal.xmm); } break;
                          default: cc.ret();  break;
                        }
                        goto comp_end;
                        break;
      case BC_LOADIVAR:  
      case BC_LOADSVAR:
                         { Mem varValAddr = ptr(cc.argGP(BP_ARG), sizeof(int64_t) * (*bc.var_++)); 
                           ssaStack.push_back(newSSAGPVar(varValAddr, cc)); } break;
      case BC_LOADDVAR:  { Mem varValAddr = ptr(cc.argGP(BP_ARG), sizeof(int64_t) * (*bc.var_++)); 
                           ssaStack.push_back(newSSAXMMVar(varValAddr, cc)); } break; 
      case BC_STORESVAR:
      case BC_STOREIVAR: { AnyVar val = ssaStack.back(); 
                           cc.mov(ptr(cc.argGP(BP_ARG), sizeof(int64_t) * (*bc.var_++)), *val.gp);  
                           cc.unuse(*val.gp); ssaStack.pop_back(); } break;
      case BC_STOREDVAR: { AnyVar val = ssaStack.back(); 
                           cc.movq(ptr(cc.argGP(BP_ARG), sizeof(int64_t) * (*bc.var_++)), *val.xmm);
                           cc.unuse(*val.xmm); ssaStack.pop_back(); } break;
      case BC_LOADIVAR0: { throw new TranslationException("BC_LOADIVAR0 is not implemented JIT", 0);
                         } break;
      case BC_LOADDVAR0: { throw new TranslationException("BC_LOADDVAR0 is not implemented JIT", 0);
                         } break;
      case BC_STOREIVAR0:{ throw new TranslationException("BC_STOREIVAR0 is not implemented JIT", 0);
                         } break;
      case BC_STOREDVAR0:{ throw new TranslationException("BC_STOREDVAR0 is not implemented JIT", 0);
                         } break;
      case BC_JA:        { ByteCodeElem target; target.byte_ = bc.byte_ + *bc.jmp_++; 
                           if (bcAddrToLabel.find(target) == bcAddrToLabel.end()) {
                             bcAddrToLabel[target] = cc.newLabel();
                           }
                           cc.jmp(bcAddrToLabel[target]); } break; 
#define GEN_IFICMP(instr) \
                         { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); \
                           AnyVar _tos2 = ssaStack.back(); ssaStack.pop_back(); \
                           ByteCodeElem target; target.byte_ = bc.byte_ + *bc.jmp_++; \
                           cc.cmp(*_tos1.gp, *_tos2.gp); \
                           if (bcAddrToLabel.find(target) == bcAddrToLabel.end()) { \
                             bcAddrToLabel[target] = cc.newLabel(); \
                           } \
                           switch(instr) {\
                             case BC_IFICMPNE:\
                                              cc.jne(bcAddrToLabel[target]); \
                             break;\
                             case BC_IFICMPE:\
                                             cc.je(bcAddrToLabel[target]); \
                             break;\
                             case BC_IFICMPL:\
                                             cc.jl(bcAddrToLabel[target]); \
                             break;\
                             case BC_IFICMPLE:\
                                              cc.jle(bcAddrToLabel[target]); \
                             break;\
                             case BC_IFICMPG:\
                                             cc.jg(bcAddrToLabel[target]); \
                             break;\
                             case BC_IFICMPGE:\
                                              cc.jge(bcAddrToLabel[target]); \
                             break;\
                             default:\
                                     break;\
                           }\
                           cc.unuse(*_tos1.gp); cc.unuse(*_tos2.gp); \
                           if (*bc.instr_ == BCA_LOGICAL_OP_RES) {\
                             ssaStack.push_back(newSSAGPVar(cc));\
                           }\
                         }

      case BC_IFICMPNE:  GEN_IFICMP(BC_IFICMPNE); break;
      case BC_IFICMPE :  GEN_IFICMP(BC_IFICMPE);  break;
      case BC_IFICMPG :  GEN_IFICMP(BC_IFICMPG);  break; 
      case BC_IFICMPGE:  GEN_IFICMP(BC_IFICMPGE); break; 
      case BC_IFICMPL :  GEN_IFICMP(BC_IFICMPL);  break; 
      case BC_IFICMPLE:  GEN_IFICMP(BC_IFICMPLE); break; 

      case BC_DADD:      { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back();
                           cc.addsd(*_tos2.xmm, *_tos1.xmm);
                           cc.unuse(*_tos1.xmm); }   break; 
      case BC_IADD:      { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back();
                           cc.add(*_tos2.gp, *_tos1.gp);
                           cc.unuse(*_tos1.gp); }   break; 
      case BC_DSUB:      { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back(); ssaStack.pop_back();
                           cc.subsd(*_tos1.xmm, *_tos2.xmm);
                           ssaStack.push_back(_tos1);
                           cc.unuse(*_tos2.xmm); }   break;
      case BC_ISUB:      { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back(); ssaStack.pop_back();
                           cc.sub(*_tos1.gp, *_tos2.gp);
                           ssaStack.push_back(_tos1);
                           cc.unuse(*_tos2.gp); }   break;
      case BC_DMUL:      { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back();
                           cc.mulsd(*_tos2.xmm, *_tos1.xmm);
                           cc.unuse(*_tos1.xmm); }   break; 
      case BC_IMUL:      { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back();
                           cc.imul(*_tos2.gp, *_tos1.gp);
                           cc.unuse(*_tos1.gp); }   break; 
      case BC_DDIV:      { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back(); ssaStack.pop_back();
                           cc.divsd(*_tos1.xmm, *_tos2.xmm);
                           ssaStack.push_back(_tos1);
                           cc.unuse(*_tos2.xmm); }   break; 
      case BC_IDIV:      { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back(); ssaStack.pop_back();
                           /*FIXME MAYBE compiler generates invalid code for idiv
                            *FIXME using hack - slow call to compiler's func */
                           /*GPVar _tmp(cc.newGP());
                           cc.mov(_tmp, imm(0));
                           cc.idiv_lo_hi(*_tos1.gp, _tmp, *_tos2.gp);
                           ssaStack.push_back(_tos1);
                           cc.unuse(*_tos2.gp); cc.unuse(_tmp);*/
                           
                           AnyVar _func_res = newSSAGPVar(cc);
                           ECall* _call = cc.call(imm((size_t)idiv));
                           _call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, int64_t, int64_t>());
                           _call->setArgument(0, *_tos1.gp);
                           _call->setArgument(1, *_tos2.gp);
                           _call->setReturn(*_func_res.gp);
                           ssaStack.push_back(_func_res);
                           cc.unuse(*_tos1.gp); cc.unuse(*_tos2.gp);
                         }   break;
      case BC_IMOD:      { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back(); ssaStack.pop_back();
                           /*FIXME MAYBE compiler generates invalid code for imod
                            *FIXME using hack - slow call to compiler's func */
                           /*AnyVar _mod = newSSAGPVar(cc);
                           cc.mov(*_mod.gp, imm(0));
                           cc.idiv_lo_hi(*_tos1.gp, *_mod.gp, *_tos2.gp);
                           ssaStack.push_back(_mod);
                           cc.unuse(*_tos1.gp); cc.unuse(*_tos2.gp);*/
                           
                           AnyVar _func_res = newSSAGPVar(cc);
                           ECall* _call = cc.call(imm((size_t)imod));
                           _call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, int64_t, int64_t>());
                           _call->setArgument(0, *_tos1.gp);
                           _call->setArgument(1, *_tos2.gp);
                           _call->setReturn(*_func_res.gp);
                           ssaStack.push_back(_func_res);
                           cc.unuse(*_tos1.gp); cc.unuse(*_tos2.gp);
                         } break;
      case BC_SWAP:      { throw new TranslationException("BC_SWAP is not implemented JIT", 0);
                         } break;
      case BC_DCMP:      { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back(); ssaStack.pop_back();
                           AnyVar _res  = newSSAGPVar(cc);
                           //FIXME: try to make it in assembler
                           /*GPVar _tmp1(cc.newGP());
                             GPVar _tmp2(cc.newGP());
                             cc.movq(_tmp1, *_tos1.xmm);
                             cc.movq(_tmp2, *_tos2.xmm);
                             cc.cmp(_tmp1, _tmp2);
                             cc.unuse(_tmp1); cc.unuse(_tmp2);
                           //cc.ucomisd(*_tos1.xmm, *_tos2.xmm);
                           AsmJit::Label lblGt = cc.newLabel();
                           AsmJit::Label lblLs = cc.newLabel();
                           AsmJit::Label lblEnd = cc.newLabel();
                           cc.jl(lblLs);
                           cc.jg(lblGt);
                           cc.mov(*_res.gp, imm(0));
                           cc.jmp(lblEnd);
                           cc.bind(lblLs);
                           cc.mov(*_res.gp, imm(-1));
                           cc.jmp(lblEnd);
                           cc.bind(lblGt);
                           cc.mov(*_res.gp, imm(1));
                           cc.bind(lblEnd);*/
                           ECall* _call = cc.call(imm((size_t)dcmp));
                           _call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, double, double>());
                           _call->setArgument(0, *_tos1.xmm);
                           _call->setArgument(1, *_tos2.xmm);
                           _call->setReturn(*_res.gp);

                           ssaStack.push_back(_res);
                           cc.unuse(*_tos1.xmm); cc.unuse(*_tos2.xmm); } break; 
      case BC_CALL:      { 
                           uint16_t id = (uint16_t)*(bc.var_++);
                           TranslatableFunction& func = executable.getMetaData()[id];
                           AnyVar _func_addr = newSSAGPVar((size_t)(cFuncPtrs + id), cc);
                           AnyVar _bp = newSSAGPVar(cc);
                           AnyVar _sp = newSSAGPVar(cc);
                           cc.mov(*_bp.gp, cc.argGP(SP_ARG));
                           cc.mov(*_sp.gp, cc.argGP(SP_ARG));
                           cc.sub(*_bp.gp, imm(func.getFrameSize() * sizeof(int64_t)));
                           ECall* _call = cc.call(ptr(*_func_addr.gp));

                           switch(func.getProto().typeInfo.returnType) {
                             case VT_INT: case VT_STRING:
                               {AnyVar _func_res = newSSAGPVar(cc);
                                 _call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, void*, void*>());//ret bp sp
                                 _call->setReturn(*_func_res.gp);
                                 ssaStack.push_back(_func_res);}
                               break;
                             case VT_DOUBLE:
                               {AnyVar _func_res = newSSAXMMVar(cc);
                                 _call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<double, void*, void*>());
                                 _call->setReturn(*_func_res.xmm);
                                 ssaStack.push_back(_func_res);}
                               break;
                             default:
                               _call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<void, void*, void*>());
                               break;
                           }
                           _call->setArgument(BP_ARG, *_bp.gp);
                           _call->setArgument(SP_ARG, *_sp.gp);
                           //cc.sub(cc.argGP(SP_ARG), imm(func.getFrameSize()));
                           cc.unuse(*_func_addr.gp); cc.unuse(*_bp.gp); cc.unuse(*_sp.gp);
                         } break; 
      case BC_CALLNATIVE:{
                           uint16_t id = (uint16_t)*(bc.var_++);
                           MyNativeFunction *native_func = 
                             executable.nativeFuncById(id);
                           //create native func arg variables;
                           typedef std::pair<VarType, AnyVar> Param;
                           std::vector<Param> params;
                           Signature::const_reverse_iterator sit, sit_end;
                           sit = native_func->getSignature().rbegin();
                           sit_end = native_func->getSignature().rend();
                           --sit_end;//last is return type
                           
                           for(; sit != sit_end; ++sit) {
                             Param param;
                             param.first = sit->first;
                             switch(param.first) {
                               case VT_DOUBLE:
                                param.second = newSSAXMMVar(cc);
                               break;
                               default:
                                param.second = newSSAGPVar(cc);
                               break;
                             }
                             params.push_back(param);
                           }

                           FunctionBuilderX fBuilder;
                           for(size_t i = 0; i != params.size(); ++i) {
                             switch(params[i].first) {
                               case VT_DOUBLE:
                                 fBuilder.addArgument<double>();
                                 cc.movq(*params[i].second.xmm,
                                     ptr(cc.argGP(SP_ARG), -sizeof(uint64_t) * (params.size() - i))
                                     );
                                 break;
                               default:
                                 fBuilder.addArgument<int64_t>();
                                 cc.mov(*params[i].second.gp,
                                     ptr(cc.argGP(SP_ARG), -sizeof(uint64_t) * (params.size() - i))
                                     );
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
                             switch(params[i].first) {
                               case VT_DOUBLE:
                                 _call->setArgument(i, *params[i].second.xmm);
                                 break;
                               default:
                                 _call->setArgument(i, *params[i].second.gp);
                                 break;
                             }
                           }
                           AnyVar retVal;
                           retVal.gp = NULL;
                           switch(native_func->getReturnType()) {
                             case VT_DOUBLE:{
                                              retVal = newSSAXMMVar(cc);
                                              _call->setReturn(*retVal.xmm);
                                              break;}
                             case VT_VOID:
                                            break;
                             default:{
                                       retVal = newSSAGPVar(cc);
                                       _call->setReturn(*retVal.gp);
                                       break;}
                           }
                           if (retVal.gp != NULL) {
                             ssaStack.push_back(retVal);
                           }
                           for(size_t i = 0; i != params.size(); ++i) {
                             switch(params[i].first) {
                               case VT_DOUBLE:
                                 cc.unuse(*params[i].second.xmm);
                                 break;
                               default:
                                 cc.unuse(*params[i].second.gp);
                                 break;
                             }
                           }
                         } break; 
      case BC_RETURN:    switch(bcFunc->returnType()) {
                           case VT_INT: case VT_STRING:
                             {AnyVar res = ssaStack.back(); cc.mov(*retVal.gp, *res.gp); 
                               /*cc.ret(*retVal.gp);*/ cc.jmp(lblRet); 
                               ssaStack.pop_back(); cc.unuse(*res.gp); } break;
                           case VT_DOUBLE: 
                             {AnyVar res = ssaStack.back(); cc.movq(*retVal.xmm, *res.xmm); 
                               /*cc.ret(*retVal.xmm);*/ cc.jmp(lblRet);
                               ssaStack.pop_back(); cc.unuse(*res.xmm); } break;
                           default: /*cc.ret();*/ cc.jmp(lblRet); break;
                         }  break;
      case BCA_FCALL_BEGIN: break;
      case BCA_FCALL_END:   break;
      case BCA_VM_SPECIFIC:     ++bc.instr_; break;
      case BCA_FPARAM_CLEANUP:  ++bc.instr_; cc.sub(cc.argGP(SP_ARG), imm(sizeof(int64_t))); break;
      case BCA_FPARAM_COMPUTED: { AnyVar _param = ssaStack.back(); ssaStack.pop_back();
                                  VarType pType = (VarType)*bc.byte_++; 
                                  switch(pType) {
                                    case VT_STRING:
                                    case VT_INT:
                                      cc.mov(ptr(cc.argGP(SP_ARG)), *_param.gp);
                                      cc.unuse(*_param.gp);
                                      break;
                                    case VT_DOUBLE:
                                      cc.movq(ptr(cc.argGP(SP_ARG)), *_param.xmm);
                                      cc.unuse(*_param.xmm);
                                      break;
                                    default: break;
                                  }
                                  cc.add(cc.argGP(SP_ARG), imm(sizeof(int64_t)));
                                } break;
      case BCA_FPARAM_CLOSURE_SAVE: bc.instr_++;
                                    { GPVar val(cc.newGP());
                                      cc.mov(val, ptr(cc.argGP(SP_ARG), -sizeof(int64_t)));
                                      cc.mov(ptr(cc.argGP(BP_ARG), sizeof(int64_t) * (*bc.var_++)), val);  
                                      cc.unuse(val); 
                                      cc.sub(cc.argGP(SP_ARG), imm(sizeof(int64_t))); } break;
      default:
                                    DEBUG("Unknown instraction");
    };
  }
comp_end:
#ifdef VERBOSE
  std::cerr << std::endl << std::endl;
  std::cerr << "Binded labels: " << bcAddrToLabel.size() << std::endl;
  for(std::map<ByteCodeElem, AsmJit::Label>::iterator it = bcAddrToLabel.begin();
      it != bcAddrToLabel.end(); ++it) {
    std::cerr << it->first.instr_ - bcStart.instr_ << std::endl;
  }
  std::cerr << "Metadata labels: " << funcMetadata.getBCAddressesJumped().size() << std::endl;
  for(BCAddressesJumped::iterator it = funcMetadata.getBCAddressesJumped().begin();
      it != funcMetadata.getBCAddressesJumped().end(); ++it) {
    std::cerr << *it << std::endl;
  }
  std::cerr << std::endl << std::endl;
#endif
  cc.endFunction();
  cFuncPtrs[funcId] = (void*)cc.make();
}
