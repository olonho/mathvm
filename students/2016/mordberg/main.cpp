#include "mathvm.h"

#include <stdio.h>
#include "translator_impl.h"
#include <sys/stat.h>

#include <iostream>

using namespace mathvm;
using namespace std;

int main(int argc, char **argv) {
    string impl = CMD_INTERPRETER;
    const char *script = NULL;
    for (int32_t i = 1; i < argc; i++) {
        if (string(argv[i]) == "-j") {
            impl = CMD_JIT;
        } else if (string(argv[i]) == "-p") {
            impl = CMD_PRINTER;
        } else if (string(argv[i]) == "-t") {
            impl = CMD_TRANSLATOR;
        } else if (string(argv[i]) == "-i") {
            impl = CMD_INTERPRETER;
        } else {
            script = argv[i];
        }
    }
    Translator *translator = Translator::create(impl);

    if (translator == 0) {
        cout << "TODO: Implement translator factory in translator.cpp!!!!" << endl;
        return 1;
    }

    const char *expr = "double x; double y;"
            "x += 8.0; y = 2.0;"
            "print('Hello, x=',x,' y=',y,'\n');";
    bool isDefaultExpr = true;

    if (script != NULL) {
        expr = loadFile(script);
        if (expr == 0) {
            printf("Cannot read file: %s\n", script);
            return 1;
        }
        isDefaultExpr = false;
    }

    Code *code = 0;

    Status *translateStatus = translator->translate(expr, &code);
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
                       "error '%s'\n",
               line, offset,
               translateStatus->getErrorCstr());
    } else {
        if (impl == CMD_PRINTER) {
            // nothing to do
        } else if (impl == CMD_TRANSLATOR) {
            code->disassemble();
            delete code;
        } else if (impl == CMD_INTERPRETER) {
            auto args = vector<Var *>();
            auto execStatus = code->execute(args);
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

    if (!isDefaultExpr) {
        delete[] expr;
    }

    return 0;
}
