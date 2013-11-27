//
//  main.cpp
//
//  Created by Vadim Lomshakov.
//  Copyright (c) 2013 spbau. All rights reserved.
//

#include "mathvm.h"
#include "parser.h"
#include "codePrinter.h"


int main(int argc, char** argv) {
  using namespace mathvm;
  using namespace std;

  const char* script = NULL;

  if (argc >= 2) {
    script = argv[1];
  } else {
    printf("Input script name\n");
    return 1;
  }

  const char* text = loadFile("/Users/vlomshakov/Documents/vadik/VirtualMachine/mathvm/students/2013/lomshakov/ArrayExtension/ext_tests/array.mvm");//script);
  if (text == 0) {
    printf("Cannot read file: %s\n", script);
    return 1;
  }

//  const char* text = ""
//  "int[] a = new int[11];"
//  "a[2] = 100;"
//  "a = new int[22];"
//  "int z = a[2];"
//  "function int[][] foo (int[] a, double[][] z) {}"
//  ;

  Parser parser = Parser();

  Status* parserStatus = parser.parseProgram(text);
  if (parserStatus == 0 || parserStatus->isOk()) {
    CodePrinter inverseTranslation = CodePrinter(cout);
    inverseTranslation.print(parser.top());
  } else {
    printf("%s in position %d ", parserStatus->getError().c_str(), parserStatus->getPosition());
  }

  return 0;
}
