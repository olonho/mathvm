#include <iostream>
#include <vector>

#include "MyBytecodeTranslator.h"
#include "MyBytecodeInterpretator.h"


int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "need nvm filename" << std::endl;
        return -1;
    }

    const char * source = mathvm::loadFile(argv[1]);
    if (source == 0) {
        std::cout << "wrong nvm filename" << std::endl;
        return -1;
    }

    mathvm::Code* code = new mathvm::MyBytecodeInterpretator();
    mathvm::Translator* translator = mathvm::Translator::create();
    mathvm::Status* status = translator->translate(source, &code);
    if (status->isOk()) {
        std::vector<mathvm::Var*> vars;
        code->execute(vars);
    }
}
