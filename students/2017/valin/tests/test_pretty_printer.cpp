#include "pretty_printer.h"
#include "parser.h"
#include "ast.h"

#include <fstream>
#include <string>

int main(int argc, char ** argv)
{
	if (argc != 2) {
		std::cerr << "wrong number of arguments\n";
		return 1;
	}

	mathvm::Parser parser;
	parser.parseProgram(mathvm::loadFile(argv[1]));
	my::AstPrinter::pretty_printer(parser.top(), std::cout);

	return 0;
}
