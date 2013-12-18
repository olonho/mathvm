#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "BcTranslator.h"

void printUsage(const std::string& exe)
{
	std::cout << "Usage: " << exe << " <source>" << std::endl;
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		printUsage(argv[0]);
		return 0;
	}

	std::ifstream in(argv[1]);
	if (!in) {
		std::cerr << "Cannot read source file: " << argv[1] << std::endl;
		return 1;
	}

	std::ostringstream progstream;
	progstream << in.rdbuf();
	in.close();
	std::string prog(progstream.str());

	BcTranslator translator;

	mathvm::Code* code = 0;
	mathvm::Status* status = translator.translate(prog, &code);
	if (status && status->isError()) {
		uint32_t position = status->getPosition();
		uint32_t line = 0, offset = 0;
		mathvm::positionToLineOffset(prog, position, line, offset);

		std::cerr << "Cannot translate expression: expression at " <<
		          line << "," << offset << " error '" << status->getError() << "'" << std::endl;

		delete status;
		return 1;
	}

	std::vector<mathvm::Var*> vars;
	status = code->execute(vars);
	if (status && status->isError()) {
		std::cerr << "Cannot execute expression: error: " << status->getError() << std::endl;
	}

	delete status;
	delete code;
	return 0;
}
