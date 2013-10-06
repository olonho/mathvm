/* 
 * File:   main.cpp
 * Author: stasstels
 *
 * Created on September 22, 2013, 4:20 PM
 */

#include <iostream>
#include <memory>

#include "mathvm.h"
#include "parser.h"

#include "PrintVisitor.h"



int main(int argc, char** argv) {

    const char* program = 0;

    if (argc == 2) {
        program = argv[1];
    } else {
        std::cerr << "Usage: " << argv[0] << " program.mvm" << std::endl;
        exit(1);
    }
    
    const char* text = mathvm::loadFile(program);
    
    if(text == 0) {
        std::cerr << "Can't load file " << program << std::endl;
        exit(1);
    }
    
    
    mathvm::Parser parser;
    mathvm::Status* status = parser.parseProgram(text);
    
    if(status != 0 && status -> isError()) {
        std::cerr << "Parser Error: " << status-> getError() << std::endl;
        exit(1);
    }
    
    mathvm::PrintVisitor printVisitor(std::cout);
    printVisitor.visitBlockNodeInside(parser.top() -> node() -> body());
}

