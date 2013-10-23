#include "printVisitor.h"
#include "parser.h"
#include <iostream>

int main(int argc, char const *argv[]) {
	if (argc != 2) {
		std::cout << "USAGE: " << std::endl;
		std::cout << "       " << argv[0] << " <mvm file name>" << std::endl;
	} else {
		char* program = mathvm::loadFile(argv[1]);
		if (!program) {
			std::cerr << "failed to load file " << argv[1] << std::endl;
			std::cerr << "terminating" << std::endl;
			return -1;
		}
		mathvm::Parser parser;
		Status* status = parser.parseProgram(program);
		if (status && status->isError()) {
			std::cerr << "error " << status->getError() << std::endl;
			return -2;
		}

		PrintVisitor visitor(std::cout);
		parser.top()->node()->visit(&visitor);

		delete status;
		return 0;
	}
}