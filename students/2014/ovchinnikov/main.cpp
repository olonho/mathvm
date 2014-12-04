#include "mathvm.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>

using namespace mathvm;
using namespace std;

#define IMPLEMENTATION "translator"

int main(int argc, char *argv[]) {
    string impl = IMPLEMENTATION;
    string script;

    for (int i = 1; i < argc; i++) {
        if (string(argv[i]) == "-j") {
            impl = "jit";
        } else {
            script = argv[i];
        }
    }

    std::string expr;
    {
        std::ifstream input_file(script);
        std::stringstream buffer;
        buffer << input_file.rdbuf();
        expr = buffer.str();
    }

    shared_ptr<Translator> translator(Translator::create(impl));
    if (!translator) {
        return 1;
    }

    Code *codeP = 0;
    shared_ptr<Status> translateStatus(translator->translate(expr, &codeP));
    shared_ptr<Code> code(codeP);
    if (translateStatus->isError()) {
        assert(!code);
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        cout << "Cannot translate expression: expression at " << line << "," << offset << "; "
             << "error '" << translateStatus->getError() << "'" << endl;
    } else {
        assert(code);
        vector<Var *> vars;
        shared_ptr<Status> execStatus(code->execute(vars));
        if (execStatus->isError()) {
            cout << "Cannot execute expression: error: " << execStatus->getError() << endl;
        }
    }

    return 0;
}
