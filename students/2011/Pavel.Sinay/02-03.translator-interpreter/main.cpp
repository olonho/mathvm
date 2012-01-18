#include <iostream>
#include <stdio.h>
#include "parser.h"
#include "ast.h"
#include "mathvm.h"
#include "Translator.h"
#include "Code.h"

#include "VisitorSourcePrinter.h"

int main(int argc, char **argv) {
	std::cout << "Source code translator" << std::endl;

	if (argc < 2) {
		std::cerr << "Specify file to process\n";
		return 1;
	}

	PSTranslator translator;
	char* src_code = mathvm::loadFile(argv[1]);

	mathvm::Code* code = 0;

	mathvm::Status* translateStatus = translator. translate(src_code, &code);

	if (translateStatus->isError()) {
		uint32_t position = translateStatus->getPosition();
		uint32_t line = 0, offset = 0;
		mathvm::positionToLineOffset(src_code, position, line, offset);
		printf("Cannot translate expression: expression at %d,%d; "
			"error '%s'\n", line, offset, translateStatus->getError().c_str());
	} else if (translateStatus->isError()) {
		uint32_t position = translateStatus->getPosition();
		uint32_t line = 0, offset = 0;
		mathvm::positionToLineOffset(src_code, position, line, offset);
		printf("Cannot translate expression: expression at %d,%d; "
			"error '%s'\n", line, offset, translateStatus->getError().c_str());
	} else {
		assert(code != 0);
		std::vector<mathvm::Var*> vars;

		std::cout << "--------------Executing--------------" << std::endl;
		mathvm::Status* execStatus = code->execute(vars);
		if (execStatus->isError()) {
			printf("Cannot execute expression: error: %s\n",
					execStatus->getError().c_str());
		}
		delete code;
		delete execStatus;
	}
	delete translateStatus;

	delete src_code;

	std::cout << std::endl;
	std::cout << "--------------Executing end--------------" << std::endl;
	return 0;
}
