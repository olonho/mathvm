#include <iostream> 

#include "parse.h"
#include "CodeVisitor.h"


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: show FILE" << std::endl;
        return 1;
    }
    mathvm::Parser p;
    parseFile(p, argv[1]);
    CodeVisitor v;
    v.translate(p.top());
    v.getMyCode().dump();
    return 0;
}
