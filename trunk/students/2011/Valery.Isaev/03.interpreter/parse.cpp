#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

void parseExpr(mathvm::Parser& p, const std::string& expr) {
    mathvm::Status* s = p.parseProgram(expr);
    if (s && s->isError()) {
        uint32_t position = s->getPosition();
        if (position == mathvm::Status::INVALID_POSITION) {
            fprintf(stderr, "%s\n", s->getError().c_str());
        } else {
            uint32_t line = 0, offset = 0;
            mathvm::positionToLineOffset(expr, position, line, offset);
            fprintf(stderr, "%d:%d: %s\n", line, offset, s->getError().c_str());
        }
        exit(1);
    }
    delete s;
}

void parseFile(mathvm::Parser& p, const char* file, std::string& s) {
    char* expr = mathvm::loadFile(file);
    if (expr == 0) {
        fprintf(stderr, "Cannot read file: %s\n", file);
        exit(1);
    }
    s = expr;
    delete [] expr;
    parseExpr(p, s);
}
