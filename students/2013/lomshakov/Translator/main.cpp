//
//  main.cpp
//
//  Created by Vadim Lomshakov on 9/14/13.
//  Copyright (c) 2013 spbau.
//




#include "mathvm.h"
#include <AsmJit/AsmJit.h>
#include <dlfcn.h>

using namespace mathvm;
using namespace std;
using namespace AsmJit;

//int main(int argc, char* argv[])
//{
//  void* _LIBC_Handle = dlopen("libc.dylib", RTLD_LAZY);
//  if (!_LIBC_Handle) {
//    char const * msg = dlerror();
//
//  }
//  void* sym = dlsym(_LIBC_Handle, "printf");
//  if (!sym) {
//    char const * msg = dlerror();
//
//  }
//
//
//  typedef int (*FuncType)(double);
//
//  Compiler c;
//  FileLogger logger(stderr);
//
//  c.setLogger(&logger);
//
//  AsmJit::Label funcBL = c.newLabel();
//  AsmJit::Label skip(c.newLabel());
//  AsmJit::Label funcCL(c.newLabel());
//  ECall* ctxRec;
//
//  char ar[16];
//  int64_t* p = (int64_t *)(void*) ar;
//  *(p + 1) = 100;
//
//
//  EFunction* funcA = c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder1<int, double>());
//
////    GPVar cnt(c.newGP());
////    c.mov(cnt, 1);
////    c.save(cnt);
//
//
////    GPVar i(c.newGP());
////    c.mov(i, 4);
//
////    GPVar t(c.newGP());
//  XMMVar tmp(c.argXMM(0));
//  GPVar var(c.newGP());
//  c.cvtsd2si(var, tmp);
//
////    c.mov(t, Imm((sysint_t)(void*)(ar)));
////    c.mov(var, qword_ptr(t, 8));
//
//
////    GPVar var = c.newGP();
////    ECall* ctx = c.call(funcBL);
////
////    ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<int, int>());
////    ctx->setArgument(0, i);
////    ctx->setReturn(var);
//
//    c.ret(var);
//    c.endFunction();
//
//
////  c.align(16);
////  c.bind(funcBL);
////  EFunction* funcB = c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder1<int, int>());
////  {
////    GPVar arg0(c.argGP(0));
////
////
////    c.cmp(arg0, 1);
////    c.jle(skip);
////
////    GPVar r(c.newGP());
////    ECall* pf = c.call(funcCL);
////    pf->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<int, int>());
////    pf->setArgument(0, arg0);
////    pf->setReturn(r);
////    c.mov(arg0, r);
////
////    c.bind(skip);
////    c.ret(arg0);
////    c.endFunction();
////  }
////
////  c.align(16);
////  c.bind(funcCL);
////  EFunction* funcC = c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder1<int, int>());
////  {
////    GPVar a(c.argGP(0));
////    c.sub(a, Imm(1)) ;
////    GPVar r(c.newGP());
////
////    ECall* ctx = c.call(funcBL);
////    ctx->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<int, int>());
////    ctx->setArgument(0, a);
////    ctx->setReturn(r);
////
////    c.ret(r);
////    c.endFunction();
////  }
//
//
//
//
//  FuncType f = function_cast<FuncType>(c.make());
//
//
//
//  int result = f(1.1333);
//
//  std::cout << "result: " << result <<  std::endl;
//  return 0;
//}


int main(int argc, char** argv) {
  string impl = "";
  const char* script = NULL;
  for (int32_t i = 1; i < argc; i++) {
    if (string(argv[i]) == "-j") {
      impl = "jit";
    } else {
      script = argv[i];
    }
  }

  impl = "jit";
  Translator* translator = Translator::create(impl);

  if (translator == 0) {
    cout << "TODO: Implement translator factory in translator.cpp!!!!" << endl;
    return 1;
  }

  const char* expr =
//  "int x = 13;"
//  "double d = 42.0"
//  "function void foo() { "
//  "string j = 'kitty';"
//  "int i = 13;"
//  " function void boo() {"
//  "   int i = 2;"
//  "   print(j,x, i, '\n');"
////      "foo();"
//  " }"
//  " boo();"
//  " print( i, '[foo]', j, ' hi number', x, '\n'); "
//  "}"
//  "foo();"
//  "print( d, x, 'dsd' ,17000000000000000, 'dsd');"
//  "function int zz(double d) { "
//      "return d; }"
//      ""
//  "print(zz(90.9999));"
//      "int i = 1;"
//          "i -= 10;"
//  "print(i);"
//      "function int add(int x, int y) {"
//          "    return x + y;"
//          "}"
//          ""
//          "function void doit() {"
//          "    print('Hi\n');"
//          "}"
          ""
          "function double mul5(int max, double x1, double x2, double x3, double x4, double x5) {"
          "    double r;"
          ""
          "    r = 1.0;"
          ""
//          "    if (max > 1) {"
//          "        r = r * x1;"
//          "    }"
//          ""
//          "    if (max > 2) {"
//          "        r = r * x2;"
//          "    }"
//          ""
//          "    if (max > 3) {"
//          "        r = r * x3;"
//          "    }"
//          ""
//          "    if (max > 4) {"
//          "        r = r * x4;"
//          "    }"
//          ""
//          "    if (max > 5) {"
//          "        r = r * x5;"
//          "    }"
          ""
          "    return r;"
          "}"
          ""
//          "function int fact(int n) {"
//          "    if (n < 3) {"
//          "        return n;"
//          "    }"
//          "    return fact(n-1);"
//          "}"
//          ""
//          "print(add(2, 3), '\n');"
//          "doit();"
          "print(mul5(2, 2.0, 0.0, 0.0, 0.0, 0.0), '\n');"
//          "print(mul5(4, 2.0, 3.0, 4.0, 5.0, 0.0), '\n');"
//          "print(mul5(5, 2.0, 3.0, 4.0, 5.0, 6.0), '\n');"
//          "print(fact(9),'\n');"

  ;
  bool isDefaultExpr = true;

  if (script != NULL) {
    expr = loadFile(script);
    if (expr == 0) {
      printf("Cannot read file: %s\n", script);
      return 1;
    }
    isDefaultExpr = false;
  }

  Code* code = 0;

  Status* translateStatus = translator->translate(expr, &code);
  if (translateStatus->isError()) {
    uint32_t position = translateStatus->getPosition();
    uint32_t line = 0, offset = 0;
    positionToLineOffset(expr, position, line, offset);
    printf("Cannot translate expression: expression at %d,%d; "
        "error '%s'\n",
        line, offset,
        translateStatus->getError().c_str());
  } else {
    assert(code != 0);
    vector<Var*> vars;

    if (isDefaultExpr) {
      Var* xVar = new Var(VT_DOUBLE, "x");
      Var* yVar = new Var(VT_DOUBLE, "y");
      vars.push_back(xVar);
      vars.push_back(yVar);
      xVar->setDoubleValue(42.0);
    }
    Status* execStatus = code->execute(vars);
    if (execStatus->isError()) {
      printf("Cannot execute expression: error: %s\n",
          execStatus->getError().c_str());
    } else {
      if (isDefaultExpr) {
//        printf("x evaluated to %f\n", vars[0]->getDoubleValue());
//        for (uint32_t i = 0; i < vars.size(); i++) {
//          delete vars[i];
//        }
      }
    }
    delete code;
    delete execStatus;
  }
  delete translateStatus;
  delete translator;

  if (!isDefaultExpr) {
    delete [] expr;
  }

  return 0;
}