#include "mathvm.h"
#include "ast.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>

#include "parser.h"
#include "ByteCodeGenerator.h"
#include "NativeGenerator.h"

using namespace std;
using namespace mathvm;

bool silentMode = false;

static ICodeGenerator* CreateGenerator(std::string const & name) {
	if (name.compare("native") == 0) return new NativeGenerator;
	return new ByteCodeGenerator;
}

int main(int argc, char** argv) 
{
	if (argc < 2) {
		std::cerr << "Specify file to process\n";
		return 1;
	}

	if (argc == 4 && strcmp(argv[3], "silent") == 0) {
		silentMode = true;
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
    ICodeGenerator * generator = CreateGenerator(argc >= 3 ? argv[2] : "");
    try {
      generator->Compile(parser->top());    
      Code* code = generator->GetCode();
			if (code) {
				vector<Var*> v;
				code->execute(v);
			}
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
    //delete generator;
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
#ifdef WIN32
	if (!silentMode)system("pause");
#endif
	return 0;
}
