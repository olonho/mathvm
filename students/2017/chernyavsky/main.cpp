#include "mathvm.h"

#include <cstdio>

using namespace mathvm;
using namespace std;

int main(int argc, char **argv) {
    string impl;
    const char *script = nullptr;
    for (int32_t i = 1; i < argc; i++) {
        if (string(argv[i]) == "-j") {
            impl = "jit";
        }
        if (string(argv[i]) == "-p") {
            impl = "printer";
        } else {
            script = argv[i];
        }
    }

    Translator *translator = Translator::create(impl);
    if (translator == nullptr) {
        cout << "TODO: Implement translator factory in translator.cpp!" << endl;
        return 1;
    }

    const char *expr = "double x; double y;"
            "x += 8.0; y = 2.0;"
            "print('Hello, x=',x,' y=',y,'\n');";
    bool isDefaultExpr = true;

    if (script != nullptr) {
        expr = loadFile(script);
        if (expr == nullptr) {
            printf("Cannot read file: %s\n", script);
            return 1;
        }
        isDefaultExpr = false;
    }

    Code *code = nullptr;

    Status *translateStatus = translator->translate(expr, &code);
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; error '%s'\n",
               line, offset, translateStatus->getErrorCstr());
    } else if (impl != "printer") {
        assert(code != nullptr);
        vector<Var *> vars;

        if (isDefaultExpr) {
            Var *xVar = new Var(VT_DOUBLE, "x");
            Var *yVar = new Var(VT_DOUBLE, "y");
            vars.push_back(xVar);
            vars.push_back(yVar);
            xVar->setDoubleValue(42.0);
        }

        Status *execStatus = code->execute(vars);
        if (execStatus->isError()) {
            printf("Cannot execute expression: error: %s\n", execStatus->getErrorCstr());
        } else if (isDefaultExpr) {
            printf("x evaluated to %f\n", vars[0]->getDoubleValue());
            for (auto &var : vars) {
                delete var;
            }
        }

        delete code;
        delete execStatus;
    }

    delete translateStatus;
    delete translator;

    if (!isDefaultExpr) {
        delete[] expr;
    }

    return 0;
}
