//
//  main.cpp
//
//  Created by Vadim Lomshakov on 9/14/13.
//  Copyright (c) 2013 spbau.
//




#include "mathvm.h"


using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
  string impl = "";
  const char* script = NULL;
  for (int32_t i = 1; i < argc; i++) {
    if (string(argv[i]) == "-j") {
      impl = "intepreter";
    } else {
      script = argv[i];
    }
  }
  Translator* translator = Translator::create(impl);

  if (translator == 0) {
    cout << "TODO: Implement translator factory in translator.cpp!!!!" << endl;
    return 1;
  }

  const char* expr =
//    "int r;"
//    "int r1;"
//    ""
//    "function void fib(int x) {"
//    "    if (x == 0) {"
//    "        r = 0;"
//    "        r1 = 1;"
//    "    } else {"
//    "        fib(x - 1);"
//    "        int r2;"
//    "        r2 = r + r1;"
//    "        r = r1;"
//    "        r1 = r2;"
//    "    }"
//    "}"
//    ""
//    "fib(100000);"
//    "print(r, '\n');"
//  "function double sin(double x) native 'sin';"
//  "print( sin(3.14/2.0), '\n');"
  "string s;"
  "string s1;"
  ""
  "s = 'Hello';"
  "function int strlen(string s) native 'strlen';"
  "function string malloc(int len) native 'malloc';"
  "function void free(string s) native 'free';"
  "function string strcat(string s1, string s2) native 'strcat';"
  "function string strcpy(string dst, string src) native 'strcpy';"
  ""
  "print('strlen of ', s , ' is ', strlen(s), '\n');"
  ""
  "function string concat(string s1, string s2) {"
  "    string result;"
  "    result = malloc(strlen(s1)+strlen(s2) + 1);"
  "    strcpy(result, s1);"
  "    strcat(result, s2);"
  "    return result;"
  "}"
  ""
  "s1 = concat(s, ' kitty');"
  "print('concat ', s1, '\n');"
  "free(s1);"
  ""
//  "function double sin(double x) native 'sin';"
//  "print('sin(1.0) ', sin(1.0), '\n');"
//  ""
//  "function double pow(double x, double y) native 'pow';"
//  "print('pow(8, 1/3) ', pow(8.0, 1.0/3.0), '\n');"
//  ""
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