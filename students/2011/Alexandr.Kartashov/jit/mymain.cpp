#include <iostream> 
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "mathvm.h"

using namespace mathvm;
using namespace std;

// ================================================================================

int main(int argc, char** argv) {
  Translator* translator = Translator::create();
  const char *expr;

  printf("Starting\n");

  if (argc > 1) {
    expr = loadFile(argv[1]);
    if (expr == 0) {
      printf("Cannot read file: %s\n", argv[1]);
      return 1;
    }
  } else {
    printf("Usage mvm <mvm-file>\n");
    return 1;
  }

  Code* code = 0;

  Status* translateStatus = translator->translate(expr, &code);

  if (translateStatus) {
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

    code->execute(vars);
    delete code;   
  } 

  delete translator;
  
  return 0;
}
