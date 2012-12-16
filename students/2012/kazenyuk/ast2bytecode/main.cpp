#include <iostream>
#include <string>
#include <cassert>

#include "mathvm.h"

//#define ENABLE_TRACING 1

int main(int argc, char** argv) {
    if (argc < 2 || argc > 4) {
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

    bool output_bytecode_only = (argc == 3);
    bool run_interpreter = (argc == 2);
    bool run_interpreter_tracing = (argc == 4);

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

    if (output_bytecode_only) {
        code->disassemble(std::cout, 0);
        return 0;
    }

    delete translateStatus;
    delete translator;
    delete[] text_buffer;

    if (run_interpreter || run_interpreter_tracing) {
#ifdef ENABLE_TRACING
        std::cout << "\nExecuting" << std::endl;
#endif
        std::vector<mathvm::Var*> vars;
        if (run_interpreter_tracing) {
            vars.push_back(new mathvm::Var(mathvm::VT_INT, "#__INTERPRETER_TRACING__#"));
        }
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
