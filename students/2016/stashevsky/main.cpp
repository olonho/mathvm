#include "mathvm.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

#include "vm.h"

using namespace mathvm;
using namespace std;

int main(int argc, char **argv) {
    string impl = "run";
    const char *script = NULL;
    for (int32_t i = 1; i < argc; i++) {
        if (string(argv[i]) == "-j") {
            impl = "jit";
        } else if (string(argv[i]) == "-p") {
            impl = "printer";
        } else if (string(argv[i]) == "-t") {
            impl = "translator";
        } else if (string(argv[i]) == "-r") {
            impl = "run";
        } else {
            script = argv[i];
        }
    }

    Translator *translator = Translator::create(impl);

    if (translator == 0) {
        cout << "TODO: Implement translator factory in translator.cpp!!!!" << endl;
        return 1;
    }

    assert(script != NULL);
    const char *expr = loadFile(script);

    Code *code = 0;
    Status *translateStatus = translator->translate(expr, &code);
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; error '%s'\n",
               line, offset,
               translateStatus->getErrorCstr());
    } else if (impl == "translator") {
        code->disassemble();
    } else if (impl == "run") {
        auto args = vector<Var*>();
        code->execute(args);
    }
    delete translateStatus;
    delete translator;
    delete[] expr;

    return 0;
}
