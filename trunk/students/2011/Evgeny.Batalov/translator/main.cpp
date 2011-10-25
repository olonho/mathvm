#include <iostream> 
#include <fstream>
#include <streambuf>
#include "MyTranslator.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Please supply source file" << std::endl;
        return 1;
    }
    
    std::ifstream f(argv[1]);
    std::string program((std::istreambuf_iterator<char>(f)),
                        (std::istreambuf_iterator<char>()));

    MyTranslator trans;
    mathvm::Code* code;
    mathvm::Status* status;
    status = trans.translate(program, &code);
    if (status->isOk()) {
        return 0;
    } else {
        std::cerr << "ERROR: " << std::endl;
        std::cerr << status->getError() << std::endl;
        std::cerr << "position: " << status->getPosition() << std::endl;
        return -1;
    }
}
