#include "mathvm.h"
#include "ast_printer.h"
#include "translator_impl.h"
#include "stack_machine.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

using namespace mathvm;
using namespace std;

Translator * Translator::create(string const & impl) {
    if (impl == "printer") {
        return new AstPrinter();
    }

    return new BytecodeTranslatorImpl();
}

int main(int argc, char** argv) {
    const char* script = NULL;
    const size_t max_stack_size = 512 * 1024 * 1024;
    string impl = "translate";
    for (int32_t i = 1; i < argc; i++) {
      if (string(argv[i]) == "-j") {
        impl = "jit";
      } else if (string(argv[i]) == "-p") {
        impl = "printer";
      } else if (string(argv[i]) == "-e") {
        impl = "exec";
      } else {
        script = argv[i];
      }
    }
    Translator* translator = Translator::create(impl);

    if (translator == 0) {
        return 1;
    }

    const char* expr = "double x; double y;"
                        "x += 8.0; y = 2.0;"
                        "print('Hello, x=',x,' y=',y,'\n');"
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
    } else if (impl == "translate") {
        code->disassemble();
    } else if (impl == "exec") {
        StackMachine machine(cout, max_stack_size);
        machine.execute(code);
    }
    delete translateStatus;
    delete translator;

    if (!isDefaultExpr) {
      delete [] expr;
    }

    return 0;
}
