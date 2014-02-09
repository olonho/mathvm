#include <string>
#include <iostream>

#include "parser.h"
#include "bc_translator.h"
#include "bc_interpreter.h"

using namespace mathvm;

int main(int argc, char** argv) {

	if (argc < 2) {
		std::cout << "usage: mvm source.mvm" << std::endl;
		return -1;
	}

	char* source = loadFile(argv[1]);

	Parser parser;

	if (Status* status = parser.parseProgram(source)) {
		std::cout << status->getError() << std::endl;
		return 1;
	}

	BytecodeTranslator translator(parser.top());
	CodeImpl* code = translator.run();

	//translator._bytecode->dump(std::cout);

	BytecodeInterpreter interpreter(code);
	interpreter.run(&code->_entry_point);

	return 0;
}
