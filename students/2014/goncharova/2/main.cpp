#include <iostream>
#include <fstream>
#include "mathvm.h"
#include "my_translator.h"

using namespace std;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "not enough args";
        return -1;
    }

    std::ifstream program(argv[1]);
    std::ostream* bytecode = NULL;
    if (argc > 2) {
        bytecode = new std::ofstream(argv[2]);
    }

    std::string programText((std::istreambuf_iterator<char>(program)), std::istreambuf_iterator<char>());

    Translator* translator = new BytecodeTranslatorImpl();

    Code* code;
    std::vector<Var*> params;

    Status* status = translator->translate(programText, &code);

    if (status->isOk()) {
//        std::cout << "translation successful" << "\n";
        if (argc > 2) {
            (*bytecode) << "Translated bytecode dump:\n";
            Bytecode* bc = ((BytecodeFunction*)code->functionById(0))->bytecode();
            bc->dump(*bytecode);
            bytecode->flush();
            delete bytecode;
        }
        status = code->execute(params);
        if (status->isOk()) {
//            std::cout << "Execution successful" << "\n";
        } else {
            std::cerr << "Execution error: " << status->getError() << "\n";
            if (code) {
                //delete code;
            }
            delete translator;
            return -1;
        }
    } else {
        std::cerr << "translation error: " << status->getError() << "\n";
        if (code) {
            //delete code;
        }
        delete translator;
        return -1;
    }


    //delete code;
    delete translator;
    return 0;
}