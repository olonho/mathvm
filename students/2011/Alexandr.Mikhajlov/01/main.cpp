#include "mathvm.h"
#include "ast.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "ByteCodeGenerator.h"
#include "BytecodeInterpreter.h"

using namespace std;
using namespace mathvm;


int main(int argc, char** argv) 
{
	if (argc < 2) {
		std::cerr << "Specify file to process\n";
		return 1;
	}

	char* code = mathvm::loadFile(argv[1]);
	if (code == NULL) {
		cout << "Failed to open file: " << argv[1] << endl;
		return 1;
	}

	mathvm::Parser *parser = new mathvm::Parser;
	Status* status = parser->parseProgram(code);
	if (status == NULL) 
	{
		ByteCodeGenerator * generator = new ByteCodeGenerator;
    generator->visit(parser->top());
    generator->Dump();
    cout << "-------Output--------\n";

    BytecodeInterpreter interpreter;
    *interpreter.bytecode() = *generator->GetBytecode();
    interpreter.setVarPoolSize(256);
    *interpreter.strings() = generator->GetStringsVector();
    interpreter.execute(std::vector<Var*>());
    
    delete generator;
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

  system("pause");

	return 0;
}