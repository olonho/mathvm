#include "bytecode_translator.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    const char* script = NULL;
    assert (argc > 1 && "expected mvm file");
    script = argv[1];

    Translator* translator = Translator::create("interpreter");
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
        fprintf(stderr, "Cannot translate expression: expression at %d,%d;"
                " error '%s'\n",
                line, offset,
                translateStatus->getError().c_str());
    } else {
        assert(code != 0);

        fprintf(stderr, "========BYTECODE=========\n");
        code->disassemble(cerr);
        fprintf(stderr, "========BYTECODE=========\n");

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
