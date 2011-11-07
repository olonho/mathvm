#include <iostream>
#include <asmjit/Compiler.h>
#include <asmjit/Logger.h>
#include <asmjit/MemoryManager.h>

typedef int (*MyFn)(size_t, size_t, int*);
typedef int (*SimpleFn)(int);

int const_func() {
  return 999;
}

void printD(double arg) {
  printf("%f\n", arg);
}

void printI(int arg) {
  printf("%d\n", arg);
}

int main() {
  using namespace AsmJit;

  Compiler c;
  FileLogger logger(stderr);
  c.setLogger(&logger);

  c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder3<int, size_t, size_t, int*>());
  c.getFunction()->setHint(FUNCTION_HINT_NAKED, true);
  //add to ptr
  GPVar _arg0(c.argGP(0)); 
  GPVar _arg1(c.argGP(1)); 
  GPVar _arg2(c.argGP(2)); 
  c.add(_arg0, _arg1);
  c.mov(ptr(_arg2), _arg0);
  
  c.mov(_arg2, imm(0));
  c.ret(_arg2);
  c.endFunction();
  
  MyFn fn = function_cast<MyFn>(c.make());
  
  int res;
  std::cout << &res << std::endl;
  std::cout << "ret code:" << fn(1, 2, &res);
  std::cout << " res:" << res  << std::endl;

  MemoryManager::getGlobal()->free((void*)fn);
  //And
  Compiler c1;
  FileLogger logger1(stderr);
  c1.setLogger(&logger1);
  
  c1.newFunction(CALL_CONV_DEFAULT, FunctionBuilder1<int, int64_t>());
  c1.getFunction()->setHint(FUNCTION_HINT_NAKED, true);

  GPVar _var1(c1.newGP(VARIABLE_TYPE_GPN, "LOGICAL VAR 1"));
  GPVar _var2(c1.newGP(VARIABLE_TYPE_GPN, "LOGICAL VAR 2"));
  GPVar _res(c1.newGP(VARIABLE_TYPE_INT32, "LOGICAL AND RESULT"));
  c1.mov(_var1, imm(1));
  c1.mov(_var2, imm(1));
  c1.mov(_res, _var1);
  c1.and_(_res, _var2);
  c1.ret(_res);
  c1.endFunction();
  SimpleFn fn1 = function_cast<SimpleFn>(c1.make());
  std::cout << (fn1(100) ? "True" : "False") << std::endl;

  MemoryManager::getGlobal()->free((void*)fn1);
  //XMM
  Compiler c2;
  c2.setLogger(&logger1);
  c2.newFunction(CALL_CONV_DEFAULT, FunctionBuilder2<double, double, double*>());
  //XMMVar _argXMM(c2.argXMM(0));
  GPVar _argGP(c2.argGP(0));
  GPVar _resPtr(c2.argGP(1));

  XMMVar _xmm1(c2.newXMM(VARIABLE_TYPE_XMM_1D));
  XMMVar _xmm2(c2.newXMM(VARIABLE_TYPE_XMM_1D));
  GPVar _mulGP(c2.newGP(VARIABLE_TYPE_GPQ));
  double dv = 2.5;
  c2.mov(_mulGP, imm(*((size_t*)&dv)));
  c2.movq(_xmm1,  _mulGP);
  c2.movq(_xmm2, _argGP);
  c2.addsd(_xmm1, _xmm2);
  c2.addsd(_xmm2, _xmm1);
  c2.movq(ptr(_resPtr), _xmm1);
  
  c2.ret(_xmm2);
  
  c2.endFunction();
  typedef double (*MyFn2)(double, double*);
  MyFn2 fn2 = function_cast<MyFn2>(c2.make());
  double d;
  std::cout << fn2(100, &d) << std::endl;
  std::cout << d << std::endl;

  MemoryManager::getGlobal()->free((void*)fn2);
  //Function call by ptr with displacament
  void* funcs[5];
  funcs[2] = (void*)const_func;
  Compiler c3;
  c3.setLogger(&logger1);
  c3.newFunction(CALL_CONV_DEFAULT, FunctionBuilder1<int, int>());
  typedef int (*MyFn3)(int);
  GPVar _displ(c3.argGP(0));
  GPVar _vtable(c3.newGP());
  GPVar _func_addr(c3.newGP());
  GPVar _func_res(c3.newGP());
  c3.mov(_vtable, imm((size_t)funcs));
  c3.mov(_func_addr, _vtable);
  c3.add(_func_addr, _displ);
  ECall* _call = c3.call(ptr(_func_addr));
  _call->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder0<int>());
  _call->setReturn(_func_res);
  c3.ret(_func_res);
  c3.endFunction();
  MyFn3 fn3 = function_cast<MyFn3>(c3.make());
  std::cout << fn3(2 * sizeof(void*)) << std::endl;

  MemoryManager::getGlobal()->free((void*)fn3);

  //Function with many arguments and variables (to see register and stack allocation)
  Compiler c4;
  c4.setLogger(&logger1);
  FunctionBuilderX fBuilder4;
  fBuilder4.setReturnValue<int>();
  fBuilder4.addArgument<int>();
  fBuilder4.addArgument<int>();
  fBuilder4.addArgument<int>();
  fBuilder4.addArgument<int>();
  fBuilder4.addArgument<int>();
  fBuilder4.addArgument<int>();
  fBuilder4.addArgument<int>();
  fBuilder4.addArgument<int>();
  fBuilder4.addArgument<double>();
  fBuilder4.addArgument<double>();
  fBuilder4.addArgument<double>();
  fBuilder4.addArgument<double>();
  fBuilder4.addArgument<double>();
  fBuilder4.addArgument<double>();
  fBuilder4.addArgument<double>();
  fBuilder4.addArgument<double>();
  fBuilder4.addArgument<double>();
  c4.newFunction(CALL_CONV_DEFAULT, fBuilder4);
  c4.getFunction()->setHint(FUNCTION_HINT_NAKED, true);
  GPVar _arg41(c4.argGP(0));
  GPVar _arg42(c4.argGP(1));
  GPVar _arg43(c4.argGP(2));
  GPVar _arg44(c4.argGP(3));
  GPVar _arg45(c4.argGP(4));
  GPVar _arg46(c4.argGP(5));
  GPVar _arg47(c4.argGP(6));
  GPVar _arg48(c4.argGP(7));
  GPVar _arg49(c4.argGP(8));
  GPVar _arg410(c4.argGP(9));
  GPVar _arg411(c4.argGP(10));
  GPVar _arg412(c4.argGP(11));
  GPVar _arg413(c4.argGP(12));
  GPVar _arg414(c4.argGP(13));
  GPVar _arg415(c4.argGP(14));
  GPVar _arg416(c4.argGP(15));
  GPVar _arg417(c4.argGP(16));
  //c4.unuse(_arg41);
  //c4.unuse(_arg42);
  //c4.unuse(_arg43);
  //c4.unuse(_arg44);
  //c4.unuse(_arg45);
  //c4.unuse(_arg46);
  //c4.unuse(_arg47);
  //c4.unuse(_arg48);
  //c4.unuse(_arg49);
  //c4.unuse(_arg410);
  //c4.unuse(_arg411);
  //c4.unuse(_arg412);
  //c4.unuse(_arg413);
  //c4.unuse(_arg414);
  //c4.unuse(_arg415);
  //c4.unuse(_arg416);
  //c4.unuse(_arg417);
  typedef int (*MyFn4)(int, int, int, int, int, int, int, int, 
                       double, double, double, double, double, double, double, double, double);
  
  ECall* _call1 = c4.call(imm((size_t)printI));
  _call1->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int>());
  _call1->setArgument(0, _arg41);
  
  ECall* _call2 = c4.call(imm((size_t)printI));
  _call2->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int>());
  _call2->setArgument(0, _arg42);
  
  ECall* _call3 = c4.call(imm((size_t)printI));
  _call3->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int>());
  _call3->setArgument(0, _arg43);
  
  ECall* _call4 = c4.call(imm((size_t)printI));
  _call4->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int>());
  _call4->setArgument(0, _arg44);
  
  ECall* _call5 = c4.call(imm((size_t)printI));
  _call5->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int>());
  _call5->setArgument(0, _arg45);
  
  ECall* _call6 = c4.call(imm((size_t)printI));
  _call6->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int>());
  _call6->setArgument(0, _arg46);
  
  ECall* _call7 = c4.call(imm((size_t)printI));
  _call7->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int>());
  _call7->setArgument(0, _arg47);
  
  ECall* _call8 = c4.call(imm((size_t)printI));
  _call8->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, int>());
  _call8->setArgument(0, _arg48);
  
  ECall* _call9 = c4.call(imm((size_t)printD));
  _call9->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, double>());
  _call9->setArgument(0, _arg49);
  
  ECall* _call10 = c4.call(imm((size_t)printD));
  _call10->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, double>());
  _call10->setArgument(0, _arg410);
  
  ECall* _call11 = c4.call(imm((size_t)printD));
  _call11->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, double>());
  _call11->setArgument(0, _arg411);
  
  ECall* _call12 = c4.call(imm((size_t)printD));
  _call12->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, double>());
  _call12->setArgument(0, _arg412);
  
  ECall* _call13 = c4.call(imm((size_t)printD));
  _call13->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, double>());
  _call13->setArgument(0, _arg413);
  
  ECall* _call14 = c4.call(imm((size_t)printD));
  _call14->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, double>());
  _call14->setArgument(0, _arg414);
  
  ECall* _call15 = c4.call(imm((size_t)printD));
  _call15->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, double>());
  _call15->setArgument(0, _arg415);
  
  ECall* _call16 = c4.call(imm((size_t)printD));
  _call16->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, double>());
  _call16->setArgument(0, _arg416);
 
  ECall* _call17 = c4.call(imm((size_t)printD));
  _call17->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, double>());
  _call17->setArgument(0, _arg417);  

  c4.ret(c4.argGP(0));
  c4.endFunction();
  MyFn4 fn4 = function_cast<MyFn4>(c4.make());
  fn4(0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 8);

  MemoryManager::getGlobal()->free((void*)fn3);
  //for loop calling printf
  typedef void (*Loop)(int, int, void*);
  //std::string fmtStr = "%d\n";
  Compiler c5;
  c5.setLogger(&logger1);
  c5.newFunction(CALL_CONV_DEFAULT, FunctionBuilder3<void, int, int, void*>());
  c5.getFunction()->setHint(FUNCTION_HINT_NAKED, true);
  //c5.getFunction()->setNecked(true);

  Label lblCheck = c5.newLabel();
  Label lblEnd = c5.newLabel();
  GPVar lower(c5.argGP(0));
  GPVar higher(c5.argGP(1));
  GPVar funcLoop(c5.argGP(2));
  GPVar fmtStr(c5.newGP());

  c5.mov(fmtStr, imm((size_t)"%d\n"));
  c5.bind(lblCheck);
  c5.cmp(lower, higher);
  c5.je(lblEnd);
  
  ECall* _call51 = c5.call(funcLoop);
  _call51->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<void, char*, int>());
  _call51->setArgument(0, fmtStr);
  _call51->setArgument(1, lower);
  
  c5.add(lower, imm(1));
  c5.jmp(lblCheck);
  c5.bind(lblEnd);
  c5.ret();
  c5.endFunction();
  
  Loop loop = function_cast<Loop>(c5.make());
  loop(0, 10, (void*)printf);

  MemoryManager::getGlobal()->free((void*)loop);
}
