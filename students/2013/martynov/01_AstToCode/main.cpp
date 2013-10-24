/*
 * main.cpp
 *
 *  Created on: Oct 10, 2013
 *      Author: Semen Martynov
 */

#include "mathvm.h"
#include "parser.h"
#include "AstToCodePrinter.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	const char* expr = "double x; double y;"
			"x += 8.0; y = 2.0;"
			"print('Hello, x=',x,' y=',y,'\n');";
	bool isDefaultExpr = true;

	if (argc > 1)
	{
		expr = mathvm::loadFile(argv[1]);
		if (expr == 0)
		{
			printf("Cannot read file: %s\n", argv[1]);
			return EXIT_FAILURE;
		}
		isDefaultExpr = false;
	}

	mathvm::Parser parser;
	mathvm::Status * status = parser.parseProgram(expr);

	if (status != NULL && status->isError())
	{
		uint32_t position = status->getPosition();
		uint32_t line = 0, offset = 0;
		mathvm::positionToLineOffset(expr, position, line, offset);
		printf("Cannot parse expression: expression at %d,%d; "
				"error '%s'\n", line, offset, status->getError().c_str());
	}
	else
	{
		mathvm::AstToCodePrinter cout_printer;
		cout_printer.exec(parser.top());
	}

	if (!isDefaultExpr)
	{
		delete[] expr;
	}
	delete status;

	return EXIT_SUCCESS;
}
