#include <iostream>
#include <string>
#include <cassert>

#include "mathvm.h"

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
    mathvm::Code* code = 0;

    mathvm::Translator* translator = new mathvm::BytecodeTranslatorImpl();
    mathvm::Status* translateStatus = translator->translate(text, &code);
    if (translateStatus && translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        mathvm::positionToLineOffset(text, position, line, offset);
        std::cerr << "Cannot translate expression: expression at "
                  << line << "," << offset
                  << "; error '"
                  << translateStatus->getError().c_str() << "'"
                  << std::endl;
        delete translateStatus;
        delete translator;
        delete[] text_buffer;
        return 1;
    }

    code->disassemble(std::cout, 0);

    delete translateStatus;
    delete translator;
    delete[] text_buffer;
    return 0;
}
