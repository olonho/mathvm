#include "mathvm.h"
#include "ast.h"
#include "parser.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "MyTranslator.h"

using namespace std;
using namespace mathvm;

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: lab2 <file to translate>.\n";
        return 1;
    }

    char* code = mathvm::loadFile(argv[1]);
    if (code == 0) {
        cout << "Failed to open file: " << argv[1] << endl;
        return 1;
    }

    mathvm::Parser *parser = new mathvm::Parser;
    Status* status = parser->parseProgram(code);
    if (status == 0) {
        MyTranslator * generator = new MyTranslator;
        generator->visit(parser->top());
        generator->Dump();
        delete generator;
    }
    else if (status->isError()) {
            uint32_t position = status->getPosition();
            uint32_t line = 0, offset = 0;
            positionToLineOffset(code, position, line, offset);
            printf("Cannot translate expression: expression at %d,%d; "
                   "error '%s'\n",
                   line, offset,
                   status->getError().c_str());
    }

    delete parser;
    system("pause");
    return 0;
}
