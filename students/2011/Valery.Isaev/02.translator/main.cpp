#include <iostream> 

#include "Translator.h"
#include "Dumper.h"
#include "parse.cpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: translator FILE" << std::endl;
        return 1;
    }
    mathvm::Parser p;
    parseFile(p, argv[1]);
    Dumper dumper;
    Translator t(dumper);
    mathvm::Status r = t.translate(p.top());
    if (r.isOk()) {
        std::vector<mathvm::Var*> v;
        dumper.execute(v);
    } else {
        std::cerr << r.getError() << std::endl;
    }
    return 0;
}
