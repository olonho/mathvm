#include "mathvm.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

#include "BCTranslator.h"

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
  Translator* translator = new BCTranslator();
  
  if (translator == 0) {
    cout << "TODO: Implement translator factory in translator.cpp!!!!" << endl;
    return 1;
  }
  
  const char* expr = "int i; i = 'oe';";


 /* const char*expr = "int x1; int y; int z;"
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
  "x1 = 2; y = 2; z = 2; print(foo(2), '\n'); print(x1, '\n', y, '\n', z, '\n');"
;*/

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
  //delete translator;  // TODO: fix error during Code maps cleanup
  
  if (!isDefaultExpr) {
    delete [] expr;
  }
  
  return 0;
}
