//
//  main.cpp
//
//  Created by Vadim Lomshakov on 9/14/13.
//  Copyright (c) 2013 spbau. All rights reserved.
//

#include "mathvm.h"
#include "parser.h"
#include "codePrinter.h"

#include <stdio.h>


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

  const char* text = loadFile(script);
  if (text == 0) {
    printf("Cannot read file: %s\n", script);
    return 1;
  }

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
