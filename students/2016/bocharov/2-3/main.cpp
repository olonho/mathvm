#include "mathvm.h"
#include "parser.h"

#include <iostream>
#include <memory>

using namespace mathvm;

void usage(char* name) {
    std::cout << "Usage: " << name << " FILE" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  FILE  file with mathvm source code" << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    const char* sourceCode = loadFile(filename);
    if (!sourceCode) {
        std::cerr << filename << ": can't read file" << std::endl;
        return 1;
    }

    Translator * translator = Translator::create();
    Code* code = nullptr;

    Status* translateStatus = translator->translate(sourceCode, &code);
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(sourceCode, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
               "error '%s'\n",
               line, offset,
               translateStatus->getErrorCstr());
    } else
        code->disassemble(std::cout);

    return 0;
}
