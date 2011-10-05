#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "mathvm.h"
#include "MyTranslator.h"

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Specify file to process\n";
        return 1;
    }

    char* code = mathvm::loadFile(argv[1]);
    if (code == NULL) {
        std::cout << "Failed to open file: " << argv[1] << std::endl;
        return 1;
    }
    mathvm::Parser * parser = new mathvm::Parser;
    mathvm::Status * status = parser->parseProgram(code);
    if (status == NULL) {
        MyTranslator * translator = new MyTranslator;
        translator->visit(parser->top());
        translator->Dump();
        delete translator;
    } else {
        if (status->isError()) {
            uint32_t position = status->getPosition();
            uint32_t line = 0, offset = 0;
            mathvm::positionToLineOffset(code, position, line, offset);
            printf("Cannot translate expression: expression at %d,%d; error '%s'\n", line, offset, status->getError().c_str());
		}
    }
    delete parser;
    return 0;
}
