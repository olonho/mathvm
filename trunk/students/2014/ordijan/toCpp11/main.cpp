#include "mathvm.h"
#include "CppCode.h"

#include <stdio.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    string impl = "toCpp11";
    const char* script = NULL;
    assert(argc == 2);
    script = argv[1];
    Translator* translator = Translator::create(impl);

    if (translator == 0) {
        return 1;
    }

    char* expr = 0;
    if (script != NULL) {
        expr = loadFile(script);
        if (expr == 0) {
            printf("Cannot read file: %s\n", script);
            return 1;
        }
    }

    Code* code = 0;
    Status* translateStatus = translator->translate(expr, &code);

    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        fprintf(stderr, "Cannot translate expression: expression at %d,%d;"
                " error '%s'\n",
                line, offset,
                translateStatus->getError().c_str());
    } else {
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

    delete translateStatus;
    delete translator;

    return 0;
}
