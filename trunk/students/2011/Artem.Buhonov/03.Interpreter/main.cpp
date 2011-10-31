#include "mathvm.h"
#include "ast.h"
#include <iostream>
#include "AstToBytecode.h"
#include <stdio.h>
#include "parser.h"

using namespace std;
using namespace mathvm;

int main(int argc, char** argv) 
{
	if (argc < 2) {
		std::cerr << "Specify file to process\n";
		return 1;
	}

	mathvm::Parser *parser = new mathvm::Parser;
	char* code = mathvm::loadFile(argv[1]);
	Status* status = parser->parseProgram(code);

	if (status == NULL)
	{
		ExecutableCode exCode;
		AstToBytecode generator(&exCode);
		try {
			generator.generate(parser->top());
			generator.dump();
		}
		catch(Exception &e) {
			printf("Compile error: %s\n", e.what().c_str());			
			return 1;
		}

		try {
			vector<mathvm::Var*> vars;
			exCode.execute(vars);
		}
		catch(Exception &e) {
			printf("Runtime error: %s!\n", e.what().c_str());
			return 1;
		}		
	}
	else 
	{
		if (status->isError()) {
			uint32_t position = status->getPosition();
			uint32_t line = 0, offset = 0;
			positionToLineOffset(code, position, line, offset);
			printf("Cannot translate expression: expression at %d,%d; "
				"error '%s'\n",
				line, offset,
				status->getError().c_str());
		}
	}  

	delete parser;
	return 0;
}