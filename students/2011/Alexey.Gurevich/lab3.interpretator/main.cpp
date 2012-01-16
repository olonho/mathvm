#include "mathvm.h"
#include "ast.h"
#include "bytecoder.h"
#include "my_translator.h"
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

	char* program = mathvm::loadFile(argv[1]);
	Code* code = new MyCode();

	Translator* translator = Translator::create("my_translator");
	Status* status = translator->translate(program, &code);
	if (status == NULL) {
		vector<mathvm::Var*> vars;
		Status* executeStatus = code->execute(vars);
		if (executeStatus != NULL) {
			std::cerr << "Error during execution! " << status->getError() << std::endl;
			delete executeStatus;
		}
	}
	else if (status->isError()) {
		uint32_t position = status->getPosition();
		uint32_t line = 0, offset = 0;
		positionToLineOffset(program, position, line, offset);
		std::cerr << "Cannot translate expression at "
			  << line << "," << offset << ". "
			  << "Error: " << status->getError() << std::endl;
	}

	delete status;
	delete translator;
	delete code;
	delete [] program;
	return 0;
}
