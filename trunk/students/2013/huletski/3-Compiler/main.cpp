#include "mathvm.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

#include "LokiMachCodeTranslator.h"

using namespace mathvm;
using namespace std;

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
  Translator* translator = new LokiMachCodeTranslator();
  
  if (translator == 0) {
    cout << "TODO: Implement translator factory in translator.cpp!!!!" << endl;
    return 1;
  }

  
  const char*expr ="\
  function double XXX(double x) { return 1.0 - x*x;}";
  
  /*
  const char * expr = "\
  function double sqrt(double x) native 'sqrt';\
  function int int(double d) { return 0 + d;}\
  function double double(int i) { return 0.0 + i;}\
  function int leastFactor(int n) {\
    if (n == 0) { return 0; } if (n == 1) { return 1; }\
    if (n % 2 == 0) { return 2; } if (n % 3 == 0) { return 3;} if (n % 5 == 0) { return 5; } \
    int i; i = 7; int m;\
    m = int(sqrt(double(n)));\
    while (i<=m) {\
      if (n % i==0) { return i; } if (n%(i+4)==0) { return i+4;}\
      if (n%(i+6)==0)  { return i+6;} if (n%(i+10)==0) {return i+10;}\
      if (n%(i+12)==0) {return i+12;} if (n%(i+16)==0) {return i+16;}\
      if (n%(i+22)==0) {return i+22;} if (n%(i+24)==0) {return i+24;}\
      i += 30;\
    }\
    return n;\
  }\
  \
  function void factorize(int n, int printResult)  {\
    int least; least = n;\
    if (printResult != 0) { print(n, ' = 1'); }\
    while (n > 1) {\
      least = leastFactor(n);\
      if (n % least != 0) { print('Bug!\n'); }\
      if (printResult != 0) { print(' * ', least); }\
      n = n / least;\
    }\
    if (printResult != 0) { print('\n'); }\
  }\
  \
  int n;\
  for (n in 10000000..10100000) { factorize(n, 1); }\
  ";*/
  
  /*
  const char*expr = "\
    string s; string s1;\
    s = 'Hello';\
    function int strlen(string s) native 'strlen';\
    function string malloc(int len) native 'malloc';\
    function void free(string s) native 'free';\
    function string strcat(string s1, string s2) native 'strcat';\
    function string strcpy(string dst, string src) native 'strcpy';\
    print('strlen of ', s , ' is ', strlen(s), '\n');\
    function string concat(string s1, string s2) {\
      string result;\
      result = malloc(strlen(s1)+strlen(s2) + 1);\
      strcpy(result, s1);\
      strcat(result, s2);\
      return result;\
    }\
    s1 = concat(s, ' kitty');\
    print('concat ', s1, '\n');\
    free(s1);\
  ";
  */
  /*
  const char*expr = "int x1; int y; int z;"
  "function int foo(int x) { int r;"
    "function int bar(int a) { if (a <= 0) { return z;} x = baz(); r += 1; return x + bar(a - 1); }"
    "function int baz() { int s;"
     " function int bak(int z1) {"
      "  if (z1 <= 0) { x = z1;"
       " } else { y = z1 + 1; x = -z1; }"
     "   x -= 1; int t; t = baz(); t -= 1;"
      "  if (s < 0) { s = s * -1; } x += 1;"
       " return t + z1 + bar(z1 - y); }             "
     " if (x <= 0 || y <= 0 || z <= 0) { return x + y + z; }  "
    "  s = foo(x - 1); y += s; z = bak(z - y + 1); z -= 2; return s + bar(x); }"
   " if (x == 0) { int f; f = baz(); return bar(f - 3); }"
   " r = foo(x - 1); bar(r); return r + baz(); }"
  "x1 = 2; y = 2; z = 2; print(foo(2), '\n'); print(x1, '\n', y, '\n', z, '\n');";
  */
  
  bool isDefaultExpr = true;
  
  if (script != NULL) {
    expr = loadFile(script);
    if (expr == 0) {
      printf("Cannot read file: %s\n", script);
      return 1;
    }
    isDefaultExpr = false;
  }
  
  Code *code = 0;
  Status* translateStatus = translator->translate(expr, &code);
  
  if (translateStatus && translateStatus->isError()) {
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
    if (execStatus && execStatus->isError()) {
      printf("Cannot execute expression: error: %s\n",
             execStatus->getError().c_str());
    } /*else {
      if (isDefaultExpr) {
        printf("x evaluated to %f\n", vars[0]->getDoubleValue());
        for (uint32_t i = 0; i < vars.size(); i++) {
          delete vars[i];
        }
      }
    }*/
    
    //no need to delete code since translator owns it
    delete execStatus;
  }
  delete translateStatus;
  delete translator;
  
  if (!isDefaultExpr) {
    delete [] expr;
  }
  
  return 0;
}
