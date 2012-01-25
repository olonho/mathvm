#include <iostream>
#include <fstream>
#include <iostream>

#include "parser.h"

#include "translator.hpp"


int main(int argc, char ** argv) 
{
	if (argc < 2) {
		std::cerr << "Usage: <input file> [output file]" << std::endl;
		return 1;
	}

	char* program = mathvm::loadFile(argv[1]);
	Code* code = new CodeExecutor();
	Status * status;

	Translator * translator = Translator::create("prj");
	try {
		status = translator->translate(program, &code);
	} 
	catch (TranslatorException ex) {
		std::cerr << "Translator error: " << ex.what() << std::endl;
		return 2;
	}	

	if (status == NULL) {
		if (argc == 3) {		
			std::ofstream os;
			os.open(argv[2]);

			code->disassemble(os);

			os.close();
		}
		else {
			code->disassemble(std::cout);
		}
		
	}
	else if (status->isError()) {
		uint32_t position = status->getPosition();
		uint32_t line = 0, offset = 0;
		positionToLineOffset(program, position, line, offset);
		std::cerr << "Error " << status->getError() << " at " << line << "," << offset << "." << std::endl;
	}

	delete status;
	delete translator;
	delete code;
	delete [] program;

	return 0;
}
