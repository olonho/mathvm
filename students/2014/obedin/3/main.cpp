#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include <stdexcept>
#include <SDL/SDL.h>

#include "mathvm.h"
#include "my_translator.hpp"

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    const char* script = NULL;
    for (int32_t i = 1; i < argc; ++i)
      if (string(argv[i]) != "-j")
        script = argv[i];

    if (script == NULL) {
        cerr << "Usage: " << argv[0] << " <file>" << endl;
        return 1;
    }

    const char* expr = loadFile(script);

    if (expr == 0) {
        cerr << "Can't read file: " << script << endl;
        return 1;
    }

    Translator* translator = new MyTranslator();
    Status* translateStatus = translator->translate(expr, NULL);

    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
               "error '%s'\n",
               line, offset,
               translateStatus->getError().c_str());
    }

    delete translateStatus;
    delete translator;

    return 0;
}
