#include "mathvm.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

using namespace mathvm;
using namespace std;

enum EReturnCode {
    OK                      = 0,
    WRONG_ARG_COUNT         = 1,
    CANNOT_READ_SOURCE      = 2,
    PARCER_ERROR            = 3,
    TOP_FUN_NOT_FOUND       = 4,
    TRANSLATOR_NOT_FOUND    = 5,
    TRANSLATION_ERROR       = 6,
    EReturnCode_COUNT
};

int main(int argc, char** argv) {
    const char* script = 0;
    if (argc > 1) {
        script = argv[1];
    }

    if (script == 0) {
        cout << "Usage: translate <source_file>" << endl;
        return WRONG_ARG_COUNT;
    }

    const char* expr = loadFile(script);
    if (expr == 0) {
        cout << "Cannot read file: " << script << endl;
        return CANNOT_READ_SOURCE;
    }

    Translator* translator = new BytecodeTranslatorImpl();

    Code* code = 0;

    Status* translateStatus = translator->translate(expr, &code);
    if (translateStatus == 0 || translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        cout << "Cannot translate expression: expression at " << line << ", " << offset << "; "
             << "error '" << translateStatus->getError().c_str() << "'" << endl;
    } else {
        assert(code != 0);

        code->disassemble(cout);

        delete code;
    }

    delete translateStatus;
    delete translator;

    return 0;
}
