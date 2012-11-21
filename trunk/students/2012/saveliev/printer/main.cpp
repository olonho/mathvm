#include <fstream>
#include <iostream>

#include "mathvm.h"
#include "parser.h"

#include "printer.h"

int main(int argc, char ** argv) {
	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " source_file" << std::endl;
		return 1;
	}

	const char*	source_fname = argv[1];
	const char* code = mathvm::loadFile(source_fname);
	if (code == 0) {
		std::cerr << "can't read the file" << std::endl;
	}

	mathvm::Parser parser;
	mathvm::Status * status = parser.parseProgram(code);

	if (status != 0 && status->isError()) {
		std::cerr << "failed parsing code" << std::endl;
		delete[] code;
		delete status;
		return 1;
	}

	mathvm::Printer printer(std::cout);
	printer.print(parser.top());

	delete[] code;
	delete status;
	return 0;
}