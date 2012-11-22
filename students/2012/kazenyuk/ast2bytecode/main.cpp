#include <iostream>
#include <string>
#include <cassert>

#include "mathvm.h"

//#define ENABLE_TRACING 1

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
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

#ifndef ENABLE_TRACING
    if (argc == 2) {
#endif
        code->disassemble(std::cout, 0);
#ifndef ENABLE_TRACING
    }
#endif

    delete translateStatus;
    delete translator;
    delete[] text_buffer;

    if (argc == 3) {
#ifdef ENABLE_TRACING
        std::cout << "\nExecuting" << std::endl;
#endif
        std::vector<mathvm::Var*> vars;
        mathvm::Status* exec_status = code->execute(vars);
        if (exec_status && exec_status->isError()) {
            std::cerr << "Cannot execute expression: error: "
                      << exec_status->getError()
                      << " at BCI=" << exec_status->getPosition()
                      << std::endl;
        }
    }
    return 0;
}
