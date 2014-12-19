#include "mathvm.h"
#include "bytecode_interpreter.hpp"

#include <iostream>


using namespace mathvm;
using namespace std;

int main(int argc, char **argv) {
    string impl = "translator";
    const char *script = NULL;
    for (int32_t i = 1; i < argc; i++) {
        if (string(argv[i]) == "-j") {
            impl = "jit";
        } else {
            script = argv[i];
        }
    }
    Translator *translator = 0;

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


    translator = Translator::create(impl);

    if (translator == 0) {
        cout << "TODO: Implement translator factory in translator.cpp!!!!" << endl;
        return 1;
    }


    Code *code = new InterpreterCode;
    Status *translateStatus = translator->translate(expr, &code);
    if (translateStatus->isError()) {
        printf("Cannot translate expression. "
                        "error: '%s'\n",
                translateStatus->getError().c_str());
    } else {
        std::vector<Var *> empty_vec;
        Status *executeStatus = code->execute(empty_vec);
        if (executeStatus->isError()) {
            uint32_t position = executeStatus->getPosition();
            uint32_t line = 0, offset = 0;
            positionToLineOffset(expr, position, line, offset);
            printf("Cannot interprete expression: expression at %d,%d; "
                            "error '%s'\n",
                    line, offset,
                    executeStatus->getError().c_str());
        }
        delete executeStatus;
    }

    delete translateStatus;
    delete code;
    delete translator;

    if (!isDefaultExpr) {
        delete[] expr;
    }

    return 0;
}

