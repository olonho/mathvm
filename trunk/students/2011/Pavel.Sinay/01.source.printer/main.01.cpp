#include <iostream>
#include <stdio.h>
#include "parser.h"
#include "ast.h"
#include "mathvm.h"

#include "VisitorSourcePrinter.h"

int main(int argc, char **argv) {
	std::cout << "Source code printer" << std::endl;

	if (argc < 2) {
		std::cerr << "Specify file to process\n";
		return 1;
	}

	mathvm::Parser parser;// = new mathvm::Parser;
	char* code = mathvm::loadFile(argv[1]);
	mathvm::Status* status = parser.parseProgram(code);
	if (status == NULL) {
		//PresentationVisitor *visitor = new PresentationVisitor(std::cout);
		VisitorSourcePrinter visitor;
		parser.top()->node()->visit(&visitor);
	}
	else {
		if (status->isError()) {
			uint32_t position = status->getPosition();
			uint32_t line = 0, offset = 0;
			mathvm::positionToLineOffset(code, position, line, offset);
			printf("Cannot translate expression: expression at %d,%d; "
					"error '%s'\n",
					line, offset,
					status->getError().c_str());
		}
	}

	return 0;
}
