#include "mathvm.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    string impl = "";
    const char* script = NULL;
    for (int32_t i = 1; i < argc; i++) {
      if (string(argv[i]) == "-j") {
        impl = "jit";
      }  if (string(argv[i]) == "-p") {
        impl = "printer";
      } else {
        script = argv[i];
      }
    }
    Translator* translator = Translator::create(impl);

    if (script == NULL) {
        printf("Please specify input file\n");
        return 1;
    }

    const char* expr = loadFile(script);
    if (expr == 0) {
        printf("Cannot read file: %s\n", script);
        return 1;
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
               translateStatus->getErrorCstr());
    } else {
        //code->disassemble(cerr);

        if (impl != "printer") {
            assert(code != 0);

            vector<Var*> vars;
            Status* execStatus = code->execute(vars);

            if (execStatus->isError()) {
                printf("Cannot execute expression: error: %s\n",
                        execStatus->getErrorCstr());
            }

            delete code;
            delete execStatus;
        }
    }

    delete translateStatus;
    delete translator;

    delete [] expr;

    return 0;
}
