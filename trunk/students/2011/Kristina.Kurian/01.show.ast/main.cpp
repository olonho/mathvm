#include "mathvm.h"
#include "ast.h"
#include "AstShowVisitor.h"
#include "parser.h"
#include <iostream>
#include <stdio.h>

int main(int argc, char** argv) {
    	if (argc != 2) {
        	std::cerr << "Usage: show <FILE>" << std::endl;
        	return 1;
    	}	

	mathvm::Parser* parser = new mathvm::Parser;
	char* code = mathvm::loadFile(argv[1]);

	mathvm::Status* status = parser->parseProgram(code);
	if (status == NULL) {
		AstShowVisitor* visitor = new AstShowVisitor();
		parser->top()->node()->visit(visitor);
	}
	else if (status->isError()) {
		uint32_t position = status->getPosition();
		uint32_t line = 0, offset = 0;
		mathvm::positionToLineOffset(code, position, line, offset);
	        printf("Cannot translate expression: expression at %d,%d; Error: %s\n",
        		line, offset, status->getError().c_str());
	}

	delete parser;
	return 0;
}
