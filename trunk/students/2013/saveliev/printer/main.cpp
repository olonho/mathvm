#include <fstream>
#include <iostream>

#include "mathvm.h"
#include "parser.h"

#include "printer.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " source_file" << std::endl;
        return 1;
    }

    const char* sourceFileName = argv[1];
    const char* sourceContents = mathvm::loadFile(sourceFileName);
    if (sourceContents == 0) {
        std::cerr << "can't read the file" << std::endl;
    }

    mathvm::Parser parser;
    mathvm::Status* status = parser.parseProgram(sourceContents);

    if (status != 0 && status->isError()) {
        std::cerr << "failed parsing code" << std::endl;
        return 1;
    }

    mathvm::Printer printer(std::cout);
    printer.print(parser.top());
    return 0;
}
