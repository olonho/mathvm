/*
Please, note: author of this code thanks Alexey Gurevich for his useful advises and for allowing to see his source code.
*/


#include <fstream>
#include <iostream>

#include "parser.h"

#include "translator.hpp"
#include "exec.hpp"


int main(int argc, char ** argv) 
{
	if (argc < 2) {
		std::cerr << "Usage: <input file> [output file]" << std::endl;
		return 1;
	}
	std::ofstream os;

	char* program = mathvm::loadFile(argv[1]);

	Code * code;
	if (argc == 3) {		
		os.open(argv[2]);
		code = new CodeExecutor(os);
	} else {
		code = new CodeExecutor(std::cout);
	}		

	Status * status;

	Translator * translator = Translator::create("prj");
	try {
		status = translator->translate(program, &code);
	} 
	catch (TranslatorException ex) {
		std::cerr << "Translator error: " << ex.what() << std::endl;
		return 2;
	}	

	if (status == 0) {
		vector<mathvm::Var*> vars;
		Status * executeStatus = code->execute(vars);
		if (executeStatus != 0) {
			std::cerr << "Program finished with errors: " << status->getError() << std::endl;
		}
		delete executeStatus;
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
