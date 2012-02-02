#include <iostream>
#include <fstream>

#include "parser.h"
#include "visitor.hpp"
#include "mycode.hpp"
#include "translate.hpp"


int main(int argc, char **argv)
{
	if (argc < 2) {
		std::cerr << "Usage: <input file> [output file]" << std::endl;
		return 1;
	}

	char *test = mathvm::loadFile(argv[1]);

	if (test == NULL) {
		std::cout << "Failed to open file: " << argv[1] << std::endl;
		return 1;
	}

	mathvm::Code *code;
	std::ofstream output_stream;

	if (argc == 3) {
		output_stream.open(argv[2]);
		code = new MyCode(output_stream);
	} else {
		code = new MyCode(std::cout);
	}

	mathvm::Translator *translator = mathvm::Translator::create();

	mathvm::Status *status = translator->translate(test, &code);

	if (status == NULL) {
//		std::cerr << "== bytecode: ==" << std::endl;
//		code->disassemble(std::cerr);
//		std::cerr << "== end ==\n" << std::endl;

		std::vector<mathvm::Var *> vars;
		mathvm::Status *executeStatus = code->execute(vars);
		if (executeStatus != NULL) {
			std::cerr << "Error during execution! " << status->getError() << std::endl;
			delete executeStatus;
		}
	} else if (status->isError()) {
		uint32_t position = status->getPosition();
		uint32_t line = 0, offset = 0;
		mathvm::positionToLineOffset(test, position, line, offset);
		std::cerr << "Error " << status->getError() << " at line " << line << ", offset " << offset << "." << std::endl;
	}

	if (argc == 3)
		output_stream.close();

	delete status;
	delete translator;
	delete code;
	delete[] test;
	return 0;
}
