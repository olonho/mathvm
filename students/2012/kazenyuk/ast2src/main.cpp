#include <iostream>

#include "mathvm.h"
#include "parser.h"

#include "converter.hpp"

int main(int argc, char** argv) {
    if (argc < 2 || argc > 2) {
        std::cout << "Usage: " << argv[0] << " <program source code file>"
                  << std::endl;
        return 1;
    }

    char* text_buffer = mathvm::loadFile(argv[1]);
    if (!text_buffer) {
        std::cout << "Can't read file '" << argv[0] << "'"
                  << std::endl;
        return 2;
    }

    const std::string text(text_buffer);
    mathvm::Parser parser;

    mathvm::Status* status = parser.parseProgram(text);
    if (status && status->isError()) {
        uint32_t line = 0;
        uint32_t offset = 0;
        mathvm::positionToLineOffset(text,
                                     status->getPosition(),
                                     line, offset);
        std::cerr << "Parser error: "
                  << status->getError()
                  << " at (" << line << ":" << offset << ")"
                  << std::endl;
        return 3;
    }

    mathvm_ext::Ast2SrcConverter converter(std::cout);
    converter(parser.top());

    return 0;
}
