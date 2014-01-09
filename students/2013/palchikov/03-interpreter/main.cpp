#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "BcTranslator.h"

void printUsage(const string& exe)
{
	cout << "Usage: " << exe << " <source>" << endl;
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		printUsage(argv[0]);
		return 0;
	}

	ifstream in(argv[1]);
	if (!in) {
		cerr << "Cannot read source file: " << argv[1] << endl;
		return 1;
	}

	ostringstream progstream;
	progstream << in.rdbuf();
	in.close();
	string prog(progstream.str());

	BcTranslator translator;

	Code* code = 0;
	Status* status = translator.translate(prog, &code);
	if (status && status->isError()) {
		uint32_t position = status->getPosition();
		uint32_t line = 0, offset = 0;
		positionToLineOffset(prog, position, line, offset);

		cerr << "Cannot translate expression: expression at " <<
		          line << "," << offset << " error '" << status->getError() << "'" << endl;

		delete status;
		return 1;
	}

	//code->disassemble();

	vector<Var*> vars;
	status = code->execute(vars);
	if (status && status->isError()) {
		cerr << "Cannot execute expression: error: " << status->getError() << endl;
	}

	delete status;
	delete code;
	return 0;
}
