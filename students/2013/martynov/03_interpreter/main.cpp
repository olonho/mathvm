/*
 * main.cpp
 *
 *  Created on: Dec 8, 2013
 *      Author: sam
 */
#include <stdio.h>
#include <stdlib.h>

#include "mathvm.h"
#include "parser.h"
#include "BCTranslator.h"

int main(int argc, char** argv) {
	const char* expr = "double x; double y;"
			"x += 8.0; y = 2.0;"
			"print('Hello, x=',x,' y=',y,'\n');";
	bool isDefaultExpr = true;

	if (argc > 1) {
		expr = mathvm::loadFile(argv[1]);
		if (expr == 0) {
			printf("Cannot read file: %s\n", argv[1]);
			return EXIT_FAILURE;
		}
		isDefaultExpr = false;
	}

	mathvm::Parser parser;
	mathvm::Status* parseStatus = parser.parseProgram(expr);

	if (parseStatus != NULL && parseStatus->isError()) {
		uint32_t position = parseStatus->getPosition();
		uint32_t line = 0, offset = 0;
		mathvm::positionToLineOffset(expr, position, line, offset);
		printf("Cannot parse expression: expression at %d,%d; "
				"error '%s'\n", line, offset, parseStatus->getError().c_str());
	} else {
		mathvm::BCInterpreter interpritator;
		mathvm::BCTranslator translator(&interpritator);
		mathvm::Status* translateStatus = translator.translate(parser.top());
		if (!translateStatus) {
			std::vector<mathvm::Var*> vs;
			interpritator.execute(vs);
		}
		delete translateStatus;
	}

	if (!isDefaultExpr) {
		delete[] expr;
	}
	delete parseStatus;

	return EXIT_SUCCESS;
}
