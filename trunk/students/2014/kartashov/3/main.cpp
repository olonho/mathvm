#include "mathvm.h"

#include "code_interpreter.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>
#include <memory>

using namespace mathvm;
using namespace std;

typedef std::shared_ptr<Status> StatusPtr;
typedef std::shared_ptr<Translator> TranslatorPtr;

int main(int argc, char** argv) {
    string impl = "translator";
    const char* script = NULL;
    for (int32_t i = 1; i < argc; i++) {
      if (string(argv[i]) == "-j") {
        impl = "jit";
      } else {
        script = argv[i];
      }
    }
    TranslatorPtr translator(Translator::create(impl));
    if (translator == 0) {
        cout << "TODO: Implement translator factory in translator.cpp!!!!" << endl;
        return 1;
    }

    const char* expr = nullptr;

    if (script != NULL) {
        expr = loadFile(script);
        if (expr == 0) {
            printf("Cannot read file: %s\n", script);
            return 1;
        }
    }

    if (!expr) {
      cout << "No text to process" << endl;
      return 1;
    }

    Code* code = nullptr;
    Status* status = translator->translate(expr, &code);
    StatusPtr translateStatus(status);
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
               "error '%s'\n",
               line, offset,
               translateStatus->getError().c_str());
        return 1;

    } else {
      if (code) {
        CodeInterpreter interpreter(code);
        code->disassemble();
        try {
          interpreter.execute();
        } catch(ExecutionException e) {
          cout << e.what() << endl;
          return 1;
        }
      } else {
        cout << "No code to interpret" << endl;
        return 1;
      }
    }

    return 0;
}
