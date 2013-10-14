#include "parser.h"
#include "ast_print_visitor.h"

#include <iostream>

int main(int argc, char *argv[]) {
    if(argc != 2) {
        std::cerr << "Usage: astprint <input.mvm>" << std::endl;
        return -1;
    }

    char *code = mathvm::loadFile(argv[1]);
    if (!code) {
        std::cerr << "Unable to open file: " << argv[1] << std::endl;
        return -1;
    }

    mathvm::Parser p;
    mathvm::Status *status = p.parseProgram(code);
    if (status && status->isError()) {
        std::cerr << "Unable to parse file " << argv[1] << ": " << status->getError() << std::endl;
        return -1;
    }

    AstPrintVisitor c(cout);
    c.printCode(p.top());
    return 0;
}
