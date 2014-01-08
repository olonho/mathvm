#include "parser.h"
#include "ast_printer.h"

#include <string>
#include <iostream>

using namespace mathvm;

int main(int argc, char** argv) {

	if (argc < 2) {
		std::cout << "usage: ast_print source.mvm" << std::endl;
		return -1;
	}

	char *source = loadFile(argv[1]);

	Parser parser;

	if (Status * status = parser.parseProgram(source)) {
		std::cout << status->getError() << std::endl;
		return 1;
	}

	AstPrinter printer(std::cout);
	printer.startVisiting(parser.top());


	return 0;
}
