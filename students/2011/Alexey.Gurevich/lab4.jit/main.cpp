#include "mathvm.h"
#include "ast.h"
//#include "../vm/jit.h"
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
	Code* code = new MyMachCodeImpl();

	Translator* translator = Translator::create("my_jit");
	Status* status = translator->translate(program, &code);
	if (status == NULL) {
		vector<mathvm::Var*> vars;
		Status* executeStatus = code->execute(vars);
		if (executeStatus != NULL && !executeStatus->isOk()) {
			std::cerr << "Error during execution: " << status->getError() << std::endl;
			delete executeStatus;
		}
	}
	else if (status->isError()) {
		std::cerr << "Error during translation and MachCode generation: " << status->getError() << std::endl;
	}

	delete status;
	delete translator;
	delete code;
	delete [] program;
	return 0;
}
