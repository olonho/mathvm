#include "mathvm.h"
#include "ast.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>

#include "parser.h"
#include "ByteCodeGenerator.h"

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
    try {
      generator->Translate(parser->top());    
      Code* code = generator->GetCode();
      code->execute(vector<Var*>());
    }
		catch (TranslationException & ex) {
      cout << "Translation ERROR";
      if (ex.where()) {
        uint32_t line = 0, offset = 0;
        positionToLineOffset(code, parser->tokens().positionOf(ex.where()->position()), line, offset);
        cout << "(" << line << ":" << offset << "): ";
      } 
      else cout << ": ";
      cout << ex.what() << endl;
    }
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

	return 0;
}
