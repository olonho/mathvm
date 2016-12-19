#include <iostream>

#include "mathvm.h"
#include "m_interpreter.h"

using namespace mathvm;
using namespace std;

int main(int argc, char **argv) {
    string impl = "interpreter";
    char const *script = nullptr;
    for (int32_t i = 1; i < argc; i++) {
        if (string(argv[i]) == "-j") {
            impl = "jit";
        } else {
            script = argv[i];
        }
    }

    Translator *translator = nullptr;
    const char* expr = nullptr;

    if (script != nullptr) {
        expr = loadFile(script);
        if (expr == nullptr) {
            printf("Cannot read file: %s\n", script);
            return 1;
        }
    } else {
        printf("Script is not provided");
        return 1;
    }

    translator = Translator::create(impl);

    Code *code = new InterpreterCode;
    Status *translate_status = translator->translate(expr, &code);
    if (translate_status->isError()) {
        printf("Cannot translate. "
               "Error: '%s'\n",
               translate_status->getErrorCstr());
    } else {
        std::vector<Var *> empty_vec;
        Status *execute_status = code->execute(empty_vec);
        if (execute_status->isError()) {
            printf("Cannot interpret. "
                   "Error: '%s'\n",
                   execute_status->getErrorCstr());
        }
        delete execute_status;
    }

    delete translate_status;
    delete code;
    delete translator;

    return 0;
}
