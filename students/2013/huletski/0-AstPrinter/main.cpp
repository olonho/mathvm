#include "mathvm.h"
#include "parser.h"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>
#include "AstPrinter.h"

using namespace mathvm;
using namespace std;

const char * DEF_EXPR =
"function void bar () {print(3, '\n');return;}function int foo () {	print(1, '\n');	return 2;}foo();bar();";

int main(int argc, char** argv) {
    const char * expr;
    if (argc > 1) {
		char const * fileName = argv[1];
        expr = loadFile(fileName);
        if (expr == 0) {
            printf("Cannot read file: %s\n", fileName);
            return 1;
        }
    } else {
		size_t defExprLen = strlen(DEF_EXPR) + 1;
		expr = new char[defExprLen]();
		//it's ok to rm const for init purposes
		strncpy((char *)expr, DEF_EXPR, defExprLen);
	}

	Parser parser;
    Status* status = parser.parseProgram(expr);
    if (status && status->isError()) {
        uint32_t position = status->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
               "error '%s'\n",
               line, offset,
               status->getError().c_str());
    } else {
		//print AST
		AstPrinter printer;
		printer.print(parser.top()->node());
    }
	
	//clean up
    delete status;
	delete [] expr;

    return 0;
}
