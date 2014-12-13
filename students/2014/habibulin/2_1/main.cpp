#include "mathvm.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

#include "my_utils.h"
#include "bytecode_interpreter.h"

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    string impl = "interpreter";
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
            "function void a(int i) {"
            "   print(i, '\n');"
            "}"
            "a(3);";
    bool isDefaultExpr = true;

//    script = "/home/mrx/Svn/MathVm/mathvm/students/2014/habibulin/2_1/testing/if.mvm";
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
        printf("Cannot translate expression at %d,%d, "
               "error: \n%s\n",
               line, offset,
               translateStatus->getError().c_str());
    } else {
        BytecodeInterpreter interpreter;
        interpreter.interpret(code);
        if(interpreter.status().isError()) {
            string cause = interpreter.status().errCause();
            size_t pos = interpreter.status().errPos();

            uint32_t line = 0, offset = 0;
            positionToLineOffset(expr, pos, line, offset);
            printf("Cannot translate expression at %d,%d, "
                   "error: \n%s\n",
                   line, offset,
                   cause.c_str());
        }
        DEBUG_MSG("interpretation finished");
    }
    delete translateStatus;
    delete translator;
    if(code) {
        delete code;
    }

    if (!isDefaultExpr) {
      delete [] expr;
    }

    return 0;
}
