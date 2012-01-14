#include "mathvm.h"
#include "ast.h"
#include "printer.h"
#include "parser.h"
#include <iostream>
#include <stdio.h>

using namespace mathvm;

int main(int argc, char** argv) 
{
	if (argc < 2) {
		std::cerr << "Specify file to process\n";
		return 1;
	}

	mathvm::Parser* parser = new mathvm::Parser;
	char* code = mathvm::loadFile(argv[1]);

	Status* status = parser->parseProgram(code);
	if (status == NULL)
	{
		Printer* visitor = new Printer();
		parser->top()->node()->visit(visitor);
		delete visitor;
	}
	else if (status->isError()) {
		uint32_t position = status->getPosition();
		uint32_t line = 0, offset = 0;
		positionToLineOffset(code, position, line, offset);
		std::cerr << "Cannot translate expression at "
			  << line << "," << offset << ". "
			  << "Error: " << status->getError() << std::endl;
	}

	delete status;
	delete parser;
	delete [] code;
	return 0;
}
