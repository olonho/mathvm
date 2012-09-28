#include "mathvm.h"
#include "parser.h"
#include "SourceByASTPrinter.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

using namespace mathvm;


int main(int argc, char** argv) {

    if(argc < 2) {
    	std::cout<<"No source file name param!";
    	return 1;
    }

    const char* expr = loadFile(argv[1]);
    if (expr == 0) {
         printf("Cannot read file: %s\n", argv[1]);
         return 1;
    }

	Parser parser;
    Status *status = parser.parseProgram(expr);
    if(status != 0){
    	std::cout<<"Cannot parse source : expression at %d,%d; "
                   "error '%s'\n",
                   status->getError().c_str();
    	delete status;
    	return 1;
    }

    SourceByASTPrinter printer;
    printer.performPrint(parser.top());

    return 0;
}