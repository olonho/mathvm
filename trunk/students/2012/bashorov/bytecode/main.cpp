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

const std::string DISASM = "disasm";
const std::string INTERPRETE = "interprete";
const std::string JIT = "jit";

int main(int argc, char** argv) {
    const char* script = 0;
    std::string mode = INTERPRETE;
    for (int32_t i = 1; i < argc; i++) {
      if (string(argv[i]) == "-d") {
        mode = "disasm";
      } else if (string(argv[i]) == "-j") {
        mode = "jit";
      } else {
        script = argv[i];
      }
    }

    if (script == 0) {
        cout << "Usage: translate [-d | -j] <source_file>" << endl;
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

        if (mode == DISASM) {
            code->disassemble(cout);
        } else if (mode == JIT) {
        } else {
            vector<Var*> t;
            code->execute(t);
        }

        delete code;
    }

    delete translateStatus;
    delete translator;

    return 0;
}
