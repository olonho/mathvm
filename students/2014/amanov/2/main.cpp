#include "mathvm.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    string impl = "";
    const char* script = NULL;
    for (int32_t i = 1; i < argc; i++) {
      if (string(argv[i]) == "-j") {
        impl = "jit";
      } else {
        script = argv[i];
      }
    }
    Translator* translator = Translator::create(impl);

    if (translator == 0) {
        cout << "TODO: Implement translator factory in translator.cpp!!!!" << endl;
        return 1;
    }

    const char* expr = "double x; double y;"
                        "x += 8.0; y = 2.0;"
                        "print('Hello, x=',x,' y=',y,'\n');"
        ;
    bool isDefaultExpr = true;

    if (script != NULL) {
        expr = loadFile(script);
        if (expr == 0) {
            printf("Cannot read file: %s\n", script);
            return 1;
        }
        isDefaultExpr = false;
    }

    Code* code = 0;

    Status* translateStatus = translator->translate(expr, &code);
    assert(translateStatus != 0);
    if (translateStatus->isError()) {
        std::cerr << "Translation error !! " << translateStatus->getError() << std::endl;
    } else if (code) {
        std::vector<Var*> vars;
        Status * interpretStatus = code->execute(vars);
        assert(interpretStatus != 0);
        if (interpretStatus->isError())
            std::cerr << "Interpretation error !! " << interpretStatus->getError() << std::endl;
        delete interpretStatus;
        delete code;
    }
    delete translateStatus;
    delete translator;

    if (!isDefaultExpr) {
      delete [] expr;
    }

    return 0;
}
