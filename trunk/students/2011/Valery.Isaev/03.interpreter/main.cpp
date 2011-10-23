#include <stdio.h> 

#include "Translator.h"
#include "parse.cpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: translator FILE\n");
        return 1;
    }
    mathvm::Parser p;
    std::string text;
    parseFile(p, argv[1], text);
    Interpreter interp;
    Translator t(&interp);
    mathvm::Status r = t.translate(p.top());
    if (r.isOk()) {
        Translator::getVarMap(p.top(), interp.varMap());
        std::vector<mathvm::Var*> v;
        interp.execute(v);
    } else {
        uint32_t line = 0, offset = 0;
        mathvm::positionToLineOffset(text, p.tokens().positionOf(r.getPosition()), line, offset);
        fprintf(stderr, "%d: %d: %s\n", line, offset + 1, r.getError().c_str());
    }
    return 0;
}
