#include <iostream> 

#include "Translator.h"
#include "Dumper.h"
#include "parse.cpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: show FILE" << std::endl;
        return 1;
    }
    mathvm::Parser p;
    parseFile(p, argv[1]);
    Dumper dumper;
    Translator t(dumper);
    mathvm::Status r = t.translate(p.top());
    if (r.isError()) {
        std::cerr << r.getError() << std::endl;
    }
    std::vector<mathvm::Var*> v;
    dumper.execute(v);
    return 0;
}
