#include <cstdio>

#include "mathvm.h"
#include "prettyprint_translator.h"
#include "bytecode_translator.h"
#include "interpreter_code.h"

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    string impl;
    const char* script = nullptr;
    for (int32_t i = 1; i < argc; i++) {
        if (string(argv[i]) == "-j") {
            impl = "jit";
        }
        if (string(argv[i]) == "-p") {
            impl = "printer";
        } else {
            script = argv[i];
        }
    }

    Translator* translator = Translator::create(impl);
    if (translator == nullptr) {
        cout << "TODO: Implement translator factory in translator.cpp!" << endl;
        return 1;
    }

    const char* expr = nullptr;
    if (script != nullptr) {
        expr = loadFile(script);
        if (expr == nullptr) {
            printf("Cannot read file: %s\n", script);
            return 1;
        }
    } else {
        printf("Usage: <filename>");
        return 1;
    }

    Code *code = nullptr;
    Status* translateStatus = translator->translate(expr, &code);
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; error '%s'\n",
               line, offset, translateStatus->getErrorCstr());
    } else if (impl != "printer") {
        assert(code != nullptr);
        vector<Var*> vars;

        Status* execStatus = code->execute(vars);
        if (execStatus->isError()) {
            printf("Cannot execute expression: error: %s\n", execStatus->getErrorCstr());
        }

        delete code;
        delete execStatus;
    }

    delete translateStatus;
    delete translator;
    delete[] expr;

    return 0;
}
