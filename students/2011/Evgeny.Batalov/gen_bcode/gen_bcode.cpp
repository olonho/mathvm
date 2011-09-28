#include <iostream> 

#include "parse.h"
#include "BCodeVisitor.h"


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: show FILE" << std::endl;
        return 1;
    }
    mathvm::Parser p;
    parseFile(p, argv[1]);
    BCodeVisitor v(std::cout);
    v.show(p.top());
    return 0;
}
