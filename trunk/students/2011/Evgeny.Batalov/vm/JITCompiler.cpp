#include "JITCompiler.h"
#include <iostream>
#include <map>
#include <stdlib.h>

void JITCompiler::compileAll() {
  size_t funcId = 0; 
  for(; funcId != executable.funcCount(); ++funcId) {
    compileFunc(funcId);
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

inline void _printf(char* fmt, char* str) {
  printf(fmt, str);
}

inline void printfD(char* fmt, double val) {
  printf(fmt, val);
}

inline void printfI(char* fmt, int64_t val) {
  printf(fmt, val);
}

void JITCompiler::compileFunc(size_t funcId) {
  using namespace mathvm;
  using namespace AsmJit;
  MyBytecodeFunction *bcFunc = executable.funcById(funcId); 
  TranslatableFunction& funcMetadata = executable.getMetaData()[funcId];
  ByteCodeElem bc;
  bc.instr_ = bcFunc->bytecode()->raw();

  Compiler cc;
  FileLogger logger(stderr);
  cc.setLogger(&logger);
  switch(funcMetadata.getProto().typeInfo.returnType) {
    case VT_INT: case VT_STRING:
      cc.newFunction(CALL_CONV_DEFAULT, FunctionBuilder2<int64_t, void*, void*>()); //ret bp sp
    break;
    case VT_DOUBLE:
      cc.newFunction(CALL_CONV_DEFAULT, FunctionBuilder2<double, void*, void*>());
    break;
    case VT_VOID:
      cc.newFunction(CALL_CONV_DEFAULT, FunctionBuilder2<void, void*, void*>());
    break;
    default:
    break;
  }
  cc.getFunction()->setHint(FUNCTION_HINT_NAKED, true);

  std::map<ByteCodeElem, AsmJit::Label> bcAddrToLabel;
  SSAStack ssaStack;
 
  bool compile = true;
  while(compile) {
    if (bcAddrToLabel.find(bc) == bcAddrToLabel.end()) {
      bcAddrToLabel[bc] = cc.newLabel();
    }
    cc.bind(bcAddrToLabel[bc]);
    switch (*bc.instr_++) {
      case BC_DLOAD:   ssaStack.push_back(newSSAXMMVar(*bc.double_++, cc)); break;
      case BC_ILOAD:   ssaStack.push_back(newSSAGPVar(*bc.int_++, cc));     break;   
      case BC_SLOAD:   ssaStack.push_back(newSSAGPVar((int64_t)copyToStringPull(executable.sConstById(*bc.str_++).c_str()), cc)); break; 
      case BC_DLOAD0:  ssaStack.push_back(newSSAXMMVar(0, cc)); break;
      case BC_ILOAD0:  ssaStack.push_back(newSSAGPVar(0, cc)); break;
      case BC_SLOAD0:  { const char* strEmpty = ""; ssaStack.push_back(newSSAGPVar((int64_t)strEmpty, cc)); } break;
      case BC_DLOAD1:  ssaStack.push_back(newSSAXMMVar(1, cc)); break;   
      case BC_ILOAD1:  ssaStack.push_back(newSSAGPVar(1, cc));  break;  
      case BC_DLOADM1: ssaStack.push_back(newSSAXMMVar(-1, cc));break; 
      case BC_ILOADM1: ssaStack.push_back(newSSAGPVar(-1, cc)); break;
      case BC_DNEG:    { AnyVar _rest = newSSAXMMVar(0, cc);
                         AnyVar _prev = ssaStack.back();
                         ssaStack.pop_back();
                         cc.subsd(*_rest.xmm, *_prev.xmm);
                         ssaStack.push_back(_rest);
                         cc.unuse(*_prev.xmm); } break;
      case BC_INEG:    cc.neg(*ssaStack.back().gp); break;
      case BC_DPRINT:  { const char* fmtStr = "%f";
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
                         AnyVar _double = newSSAXMMVar(cc);
                         cc.cvtsi2sd(*_double.xmm, *_int.gp);
                         ssaStack.pop_back();
                         ssaStack.push_back(_double);
                         cc.unuse(*_int.gp);} break; 
      case BC_D2I:       {AnyVar _int = newSSAGPVar(cc);
                          AnyVar _double = ssaStack.back();
                          cc.cvtsd2si(*_int.gp, *_double.xmm);
                          ssaStack.pop_back();
                          ssaStack.push_back(_int);
                         } break;
      case BC_POP:       {AnyVar var = ssaStack.back(); ssaStack.pop_back(); cc.unuse(*var.gp);} 
                         break; 
      case BC_STOP:      switch(bcFunc->returnType()) {
                          case VT_INT: case VT_STRING:
                            {AnyVar res = ssaStack.back(); cc.ret(*res.gp); ssaStack.pop_back();} break;
                          case VT_DOUBLE: 
                            {AnyVar res = ssaStack.back(); cc.ret(*res.xmm); ssaStack.pop_back();} break;
                          default: cc.ret();  break;
                         } compile = false; break;
      case BC_LOADIVAR:  { Mem var = Mem(cc.argGP(0), sizeof(int64_t) * (*bc.var_++)); ssaStack.push_back(newSSAGPVar(var, cc)); } break;
      case BC_LOADDVAR:  { Mem var = Mem(cc.argGP(0), sizeof(int64_t) * (*bc.var_++)); ssaStack.push_back(newSSAXMMVar(var, cc)); }  break; 
      case BC_STOREIVAR: { AnyVar val = ssaStack.back(); cc.mov(Mem(cc.argGP(0), sizeof(int64_t) * (*bc.var_++)), *val.gp);  
                           cc.unuse(*val.gp); ssaStack.pop_back(); } break;
      case BC_STOREDVAR: { AnyVar val = ssaStack.back(); cc.movq(Mem(cc.argGP(0), sizeof(int64_t) * (*bc.var_++)), *val.xmm);
                           cc.unuse(*val.xmm); ssaStack.pop_back(); } break;
      case BC_LOADIVAR0:    break;
      case BC_LOADDVAR0:    break;
      case BC_STOREIVAR0:   break;
      case BC_STOREDVAR0:   break;
      case BC_JA:        { ByteCodeElem target; target.byte_ = bc.byte_ + *bc.jmp_++; 
                           if (bcAddrToLabel.find(target) == bcAddrToLabel.end()) {
                            bcAddrToLabel[target] = cc.newLabel();
                           }
                           cc.jmp(bcAddrToLabel[target]); } break; 
      case BC_IFICMPNE:  { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back(); ssaStack.pop_back();
                           ByteCodeElem target; target.byte_ = bc.byte_ + *bc.jmp_++; 
                           cc.cmp(*_tos1.gp, *_tos2.gp);
                           if (bcAddrToLabel.find(target) == bcAddrToLabel.end()) {
                            bcAddrToLabel[target] = cc.newLabel();
                           }
                           cc.jne(bcAddrToLabel[target]);
                           cc.unuse(*_tos1.gp); cc.unuse(*_tos2.gp);}   break;
      case BC_IFICMPE :  { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back(); ssaStack.pop_back();
                           ByteCodeElem target; target.byte_ = bc.byte_ + *bc.jmp_++; 
                           cc.cmp(*_tos1.gp, *_tos2.gp);
                           if (bcAddrToLabel.find(target) == bcAddrToLabel.end()) {
                            bcAddrToLabel[target] = cc.newLabel();
                           }
                           cc.je(bcAddrToLabel[target]);
                           cc.unuse(*_tos1.gp); cc.unuse(*_tos2.gp);}   break;
      case BC_IFICMPG :  { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back(); ssaStack.pop_back();
                           ByteCodeElem target; target.byte_ = bc.byte_ + *bc.jmp_++; 
                           cc.cmp(*_tos1.gp, *_tos2.gp);
                           if (bcAddrToLabel.find(target) == bcAddrToLabel.end()) {
                            bcAddrToLabel[target] = cc.newLabel();
                           }
                           cc.jg(bcAddrToLabel[target]);
                           cc.unuse(*_tos1.gp); cc.unuse(*_tos2.gp);}   break; 
      case BC_IFICMPGE:  { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back(); ssaStack.pop_back();
                           ByteCodeElem target; target.byte_ = bc.byte_ + *bc.jmp_++; 
                           cc.cmp(*_tos1.gp, *_tos2.gp);
                           if (bcAddrToLabel.find(target) == bcAddrToLabel.end()) {
                            bcAddrToLabel[target] = cc.newLabel();
                           }
                           cc.jge(bcAddrToLabel[target]);
                           cc.unuse(*_tos1.gp); cc.unuse(*_tos2.gp);}   break; 
      case BC_IFICMPL :  { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back(); ssaStack.pop_back();
                           ByteCodeElem target; target.byte_ = bc.byte_ + *bc.jmp_++; 
                           cc.cmp(*_tos1.gp, *_tos2.gp);
                           if (bcAddrToLabel.find(target) == bcAddrToLabel.end()) {
                            bcAddrToLabel[target] = cc.newLabel();
                           }
                           cc.jl(bcAddrToLabel[target]);
                           cc.unuse(*_tos1.gp); cc.unuse(*_tos2.gp);}   break;
      case BC_IFICMPLE:  { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back(); ssaStack.pop_back();
                           ByteCodeElem target; target.byte_ = bc.byte_ + *bc.jmp_++; 
                           cc.cmp(*_tos1.gp, *_tos2.gp);
                           if (bcAddrToLabel.find(target) == bcAddrToLabel.end()) {
                            bcAddrToLabel[target] = cc.newLabel();
                           }
                           cc.jle(bcAddrToLabel[target]);
                           cc.unuse(*_tos1.gp); cc.unuse(*_tos2.gp);}   break;
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
                           GPVar _tmp(cc.newGP());
                           cc.mov(_tmp, imm(0));
                           cc.idiv_lo_hi(*_tos1.gp, _tmp, *_tos2.gp);
                           ssaStack.push_back(_tos1);
                           cc.unuse(*_tos2.gp); cc.unuse(_tmp); }   break;
      case BC_SWAP:      { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back(); ssaStack.pop_back();
                           ssaStack.push_back(_tos1); ssaStack.push_back(_tos2); } break; 
      case BC_DCMP:      { AnyVar _tos1 = ssaStack.back(); ssaStack.pop_back(); 
                           AnyVar _tos2 = ssaStack.back(); ssaStack.pop_back();
                           AnyVar _res  = newSSAXMMVar(0, cc);
                           cc.ucomisd(*_tos1.xmm, *_tos2.xmm);
                           AsmJit::Label lblGt = cc.newLabel();
                           AsmJit::Label lblEnd = cc.newLabel();
                           
                           cc.je(lblEnd); //res == 0 yet
                           cc.jg(lblGt); 
                           //less
                           cc.mov(*_res.gp, imm(-1));
                           cc.jmp(lblEnd);
                           cc.bind(lblGt);
                           cc.mov(*_res.gp, imm(1));
                           cc.bind(lblEnd);
                           ssaStack.push_back(_res);
                           cc.unuse(*_tos1.xmm); cc.unuse(*_tos2.xmm); } break; 
      case BC_CALL:      { 
                           uint16_t id = (uint16_t)*(bc.var_++);
                           TranslatableFunction& func = executable.getMetaData()[id];
                           AnyVar _func_addr = newSSAGPVar((size_t)cFuncPtrs + id, cc);
                           AnyVar _bp = newSSAGPVar(cc);
                           AnyVar _sp = newSSAGPVar(func.getFrameSize(), cc);
                           cc.mov(*_bp.gp, cc.argGP(1));
                           cc.add(*_sp.gp, cc.argGP(1));
                           ECall* _call = cc.call(ptr(*_func_addr.gp));
                           _call->setArgument(0, *_bp.gp);
                           _call->setArgument(1, *_sp.gp);

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
                           cc.unuse(*_func_addr.gp); cc.unuse(*_bp.gp); cc.unuse(*_sp.gp);
                         } break; 
      case BC_RETURN:     switch(bcFunc->returnType()) {
                            case VT_INT: case VT_STRING:
                              {AnyVar res = ssaStack.back(); cc.ret(*res.gp); ssaStack.pop_back();} break;
                            case VT_DOUBLE: 
                              {AnyVar res = ssaStack.back(); cc.ret(*res.xmm); ssaStack.pop_back();} break;
                            default: cc.ret();  break;
                          } compile = false; break;
    };
  }
  cc.endFunction();
  cFuncPtrs[funcId] = (void*)cc.make();
}
