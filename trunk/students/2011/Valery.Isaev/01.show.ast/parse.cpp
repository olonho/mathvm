#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

void parseExpr(mathvm::Parser& p, const std::string& expr) {
    mathvm::Status* s = p.parseProgram(expr);
    if (s && s->isError()) {
        uint32_t position = s->getPosition();
        uint32_t line = 0, offset = 0;
        mathvm::positionToLineOffset(expr, position, line, offset);
        fprintf(stderr, "Cannot translate expression: expression at %d,%d; "
            "error '%s'\n", line, offset, s->getError().c_str());
        exit(1);
    }
    delete s;
}

void parseFile(mathvm::Parser& p, const char* file) {
    char* expr = mathvm::loadFile(file);
    if (expr == 0) {
        fprintf(stderr, "Cannot read file: %s\n", file);
        exit(1);
    }
    parseExpr(p, expr);
}
