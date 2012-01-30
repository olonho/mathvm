#include <iostream>
#include <fstream>

#include "parser.h"

#include "printer.hpp"


int main(int argc, char ** argv) 
{
	if (argc < 2) {
		std::cerr << "Usage: <input file> [output file]" << std::endl;
		return 1;
	}

	mathvm::Parser *parser = new mathvm::Parser;
	char *code = mathvm::loadFile(argv[1]);

	mathvm::Status *status = parser->parseProgram(code);
	if (status == NULL) {
		Printer *printer;

		if (argc == 3) {		
			std::ofstream os;
			os.open(argv[2]);
			printer = new Printer(os);

			parser->top()->node()->visit(printer);

			os.close();
		} else {
			printer = new Printer(std::cout);
			parser->top()->node()->visit(printer);
		}
		
		delete printer;
	} else if (status->isError()) {
		uint32_t position = status->getPosition();
		uint32_t line = 0, offset = 0;
		mathvm::positionToLineOffset(code, position, line, offset);
		std::cerr << "Error " << status->getError() << " at line " << line << ", offset " << offset << "." << std::endl;
	}

	delete status;
	delete parser;
	delete [] code;
	return 0;
}
