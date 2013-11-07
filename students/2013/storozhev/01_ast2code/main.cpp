#include <iostream>
#include <fstream>
#include <string>

#include "parser.h"
#include "AstPrintVisitor.hpp"

int main(int argc, char** argv) {
    //открыть файл, распарсить, восстановить код по дереву
    if (argc < 2) {
        std::cout << "No input file given" << std::endl;
        return 0;
    }

    std::ifstream input(argv[1]);
    if (!input.is_open()) {
        std::cerr << "Cannot open input file" << std::endl;
        return -1;
    }
    
    std::string source((std::istreambuf_iterator<char>(input)), 
                        std::istreambuf_iterator<char>());
    mathvm::Parser parser;
    mathvm::Status* status = parser.parseProgram(source);
    if (status != NULL && status->isError()) {
        std::cerr << "Parse error" << std::endl;
       return -1; 
    }
    
    mathvm::AstPrintVisitor *visitor = new mathvm::AstPrintVisitor;
    parser.top()->node()->visit(visitor);

    delete status;
    return 0;
}
