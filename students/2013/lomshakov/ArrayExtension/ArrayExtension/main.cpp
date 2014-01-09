//
//  main.cpp
//
//  Created by Vadim Lomshakov.
//  Copyright (c) 2013 spbau. All rights reserved.
//

//#include "mathvm.h"
//#include "parser.h"
//#include "codePrinter.h"
//
//
//int main(int argc, char** argv) {
//  using namespace mathvm;
//  using namespace std;
//
//  const char* script = NULL;
//
//  if (argc >= 2) {
//    script = argv[1];
//  } else {
//    printf("Input script name\n");
//    return 1;
//  }
//
//  const char* text = loadFile("/Users/vlomshakov/Documents/vadik/VirtualMachine/mathvm/students/2013/lomshakov/ArrayExtension/ext_tests/array.mvm");//script);
//  if (text == 0) {
//    printf("Cannot read file: %s\n", script);
//    return 1;
//  }
//
////  const char* text = ""
////  "int[] a = new int[11];"
////  "a[2] = 100;"
////  "a = new int[22];"
////  "int z = a[2];"
////  "function int[][] foo (int[] a, double[][] z) {}"
////  ;
//
//  Parser parser = Parser();
//
//  Status* parserStatus = parser.parseProgram(text);
//  if (parserStatus == 0 || parserStatus->isOk()) {
//    CodePrinter inverseTranslation = CodePrinter(cout);
//    inverseTranslation.print(parser.top());
//  } else {
//    printf("%s in position %d ", parserStatus->getError().c_str(), parserStatus->getPosition());
//  }
//
//  return 0;
//}



#include "mathvm.h"

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
  Translator* translator = Translator::create(impl);

  if (translator == 0) {
    cout << "TODO: Implement translator factory in translator.cpp!!!!" << endl;
    return 1;
  }

  const char* expr =
//      "double[][] a = new double[12][11];"
//      "a[0][0] = 1.13333;"
//          "a[0][1] = 1.13333;"
//          "a[1][0] = 1.13333;"
//      "print('Hello, a[0]=', a[0][0], a[1][0], a[0][1], '\n');"
  "int[] a;"
  "int i;"
  "for (i in 0..10) {"
  " a = new int[128];"
  "}"
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
//      if (isDefaultExpr) {
//        printf("x evaluated to %f\n", vars[0]->getDoubleValue());
//        for (uint32_t i = 0; i < vars.size(); i++) {
//          delete vars[i];
//        }
//      }
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

