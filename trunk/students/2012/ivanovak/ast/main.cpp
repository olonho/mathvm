#include <iostream>
#include <string>

#include "astprinter.h"
#include "parser.h"
#include "mathvm.h"

int main(int argc, char ** argv) {    
    if (argc < 2) {
      std::cerr << "Usage: ast <fileToParse>" << std::endl;
      return 0;
    }
    
    char* buf = mathvm::loadFile(argv[1]);
    
    if(!buf) {
      std::cerr << "Error reading a file!" << std::endl;
    	return 0; 
    }
    
    std::string program_text(buf);
    
    mathvm::Parser parser;
    mathvm::Status *pStatus = parser.parseProgram(program_text);

    if(pStatus && pStatus->isError()) {
        std::cerr << pStatus->getError();
        return 0;
    }

    parser.top()->node()->visit(new ASTPrinter);
  
    return 0;
    
} 
