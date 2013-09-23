#include <iostream>

#include <parser.h>
#include "Converter.h"

int main(int argc, char const * argv[]) {
	if (argc < 2) {
		std::cerr<<"Usage: program <source file>"<<std::endl;
		return -1;
	}
	char const * filename = argv[1];
	char const * source = mathvm::loadFile(filename);
	if (source == NULL) {
		std::cerr<<"Can not read file "<<argv[1]<<std::endl;
		return -1;
	}
	mathvm::Parser parser;
	mathvm::Status * status = parser.parseProgram(source);
	if (status != NULL && status->isError()) {
		std::cerr<<"Parse file error."<<std::endl;
		return -1;
	}
	mathvm::Converter converter;
	converter.printSource(parser.top());
}
