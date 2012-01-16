#include <stdio.h> 

#include "Translator.h"
#include "MvmCode.h"
#include "parser.h"

void parseFile(mathvm::Parser& p, const char* file, std::string& s) {
    char* expr = mathvm::loadFile(file);
    if (expr == 0) {
        printf("Error: Cannot read file: %s\n", file);
        return;
    }
    s = expr;
    delete [] expr;
    
    mathvm::Status* status = p.parseProgram(s);
    if (status && status->isError()) {
        uint32_t position = status->getPosition();
        if (position == mathvm::Status::INVALID_POSITION) {
            printf("Error: %s\n", status->getError().c_str());
        } else {
            uint32_t line = 0, offset = 0;
            mathvm::positionToLineOffset(s, position, line, offset);
            printf("Cannot translate expression: expression at %d,%d; Error: %s\n",
        		line, offset, status->getError().c_str());
 
        }
        return;
    }
    delete status;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: translator FILE\n");
        return 1;
    }
    mathvm::Parser p;
    std::string text;
    parseFile(p, argv[1], text);
    MvmCode interp;
    Translator t(&interp);
    mathvm::Status r = t.translate(p.top());
    if (r.isOk()) {
        std::vector<mathvm::Var*> v;
        interp.execute(v);
    } else {
        uint32_t line = 0, offset = 0;
        mathvm::positionToLineOffset(text, p.tokens().positionOf(r.getPosition()), line, offset);
        printf("Cannot translate expression: expression at %d,%d; Error: %s\n",
        		line, offset, r.getError().c_str());
 
    }
    return 0;
}
