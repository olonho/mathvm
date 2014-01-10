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
//  const char* text = loadFile("/Users/vlomshakov/Documents/vadik/VirtualMachine/mathvm/students/2013/lomshakov/ArrayExtension/ext_tests/array_syntax.mvm");//script);
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
//  "int[] a;"
//  "int i;"
//  "for (i in 0..10) {"
//  " a = new int[128];"
//  "}"
  "int[][] aa = new int[12][12];"
   "aa[2] = new int[22];"
  ;
  bool isDefaultExpr = true;

//  script = "/Users/vlomshakov/Documents/vadik/VirtualMachine/mathvm/students/2013/lomshakov/ArrayExtension/ext_tests/array_syntax.mvm";
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

    Status* execStatus = code->execute(vars);
    if (execStatus->isError()) {
      printf("Cannot execute expression: error: %s\n",
          execStatus->getError().c_str());
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

