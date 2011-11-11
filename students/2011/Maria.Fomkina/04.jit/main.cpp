#include "mathvm.h"
#include "my_translator.h"
#include "mvm_code.h"
#include "jit.h"

#include <stdio.h>
#include <iostream> 

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
  Translator* translator = Translator::create("jit");
  
  if (translator == 0) {
    cout << "TODO: Implement translator factory in translator.cpp!!!!" << endl;
    return 1;
  }
  
  const char* expr = "double x; double y;"
      "x += 8.0; y = 2.0;" 
      "print('Hello, x=',x,' y=',y,'\n');"
      ;
  bool isDefaultExpr = true;
  
  if (argc > 1) {
    expr = loadFile(argv[1]);
    if (expr == 0) {
      printf("Cannot read file: %s\n", argv[1]);
      return 1;
    }
    isDefaultExpr = false;
  }
  
  Code* code = new MvmCode();
  
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
  
  return 0;
}
