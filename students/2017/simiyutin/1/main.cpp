#include "mathvm.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    if (argc != 2) {
        cout << "single argument is permitted!";
    }
    const char* script = argv[1];
    string impl = "printer";
    Translator* translator = Translator::create(impl);

    if (translator == nullptr) {
        cout << "TODO: Implement translator factory in translator.cpp!!!!" << endl;
        return 1;
    }

    const char* expr = "double x; double y;"
                        "x += 8.0; y = 2.0;"
                        "print('Hello, x=',x,' y=',y,'\n');"
                        "if (2 > 1) {"
                            "print('hello');"
                        "}";
    bool isDefaultExpr = true;

    if (script != nullptr) {
        expr = loadFile(script);
        if (expr == nullptr) {
            printf("Cannot read file: %s\n", script);
            return 1;
        }
        isDefaultExpr = false;
    }

    Status* translateStatus = translator->translate(expr, nullptr);
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
               "error '%s'\n",
               line, offset,
               translateStatus->getErrorCstr());
    }
    delete translateStatus;
    delete translator;

    if (!isDefaultExpr) {
      delete [] expr;
    }

    return 0;
}
