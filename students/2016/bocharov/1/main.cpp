#include "mathvm.h"
#include "parser.h"
#include "astprinter.h"

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

    Parser parser;
    std::unique_ptr<Status> status(parser.parseProgram(sourceCode));
    if (status && status->isError()) {
        std::cerr << "failed to parse " << filename << std::endl;
        std::cerr << status->getPosition() << ": " << status->getError() << std::endl;
        return 1;
    }

    AstPrinter printer(std::cout);
    parser.top()->node()->visitChildren(&printer);

    return 0;
}
