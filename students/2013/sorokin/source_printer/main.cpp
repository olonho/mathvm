#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include "parser.h"
#include "mathvm.h"
#include "source_printer.h"

using mathvm::Status;
using mathvm::AstFunction;
using std::cout;
using std::endl;

void println(std::string const & str) {
    std::cout << str << std::endl;
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        std::cerr << "wrong number of arguments. programm  takes only one argument: path to source file" << std::endl;
        return 0;
    } 

    char *buffer =  mathvm::loadFile(argv[1]);
    const std::string str(buffer);
    //println(str);
    
    mathvm::Parser parser;
    mathvm::Status* status = parser.parseProgram(str);

    if(status != NULL && !status->isOk()) {
        println(status->getError());
        return 0;       
    }

    parser.top()->node()->visit(new source_printer(cout));
    return 0;
} 
