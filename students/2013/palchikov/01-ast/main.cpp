#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "parser.h"
#include "AstPrinter.h"


void printUsage() {
	std::cout << "Usage: astprint <file>" << std::endl;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		printUsage();
		return 0;
	}

	std::ifstream in(argv[1]);
	if (!in) {
		std::cerr << "Cannot read input file." << std::endl;
		return 1;
	}

	std::ostringstream prog;
	prog << in.rdbuf();
	in.close();

	mathvm::Parser parser;
	mathvm::Status* s = parser.parseProgram(prog.str());
	if (s && s->isError()) {
		std::cerr << "Cannot parse program" << std::endl;
		std::cerr << s->getError() << std::endl;
		delete s;
		return 1;
	}

	if (s) delete s;

	AstPrinter printer(std::cout);
	printer.printBlockContent(parser.top()->node()->body());

	return 0;
}
