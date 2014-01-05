#include "mathvm.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

using namespace mathvm;
using namespace std;

#define PROD

int main(int argc, char** argv) {

    string impl = "";


    // const char* script = "tests/while.mvm";
    // const char* script = "tests/function.mvm";
    // const char* script = "tests/for.mvm";
    const char* script = "tests/expr.mvm";
    //     const char* script = "tests/mul.mvm";
    // const char* script = "tests/while.mvm";
    // const char* script = "tests/assign.mvm";
    // const char* script = NULL;

    //    const char* script = "tests/additional/function-cast.mvm";
    // const char* script = "tests/additional/ackermann.mvm";
    // const char* script = "tests/additional/fib.mvm";
    // const char* script = NULL;

    for (int32_t i = 1; i < argc; i++) {
        if (string(argv[i]) == "-j") {
            impl = "jit";
        } else {
            script = argv[i];
        }
    }
    Translator* translator = Translator::create(impl);

    const char* expr = "double x; double y;"
            "x += 8.0; y = 2.0;"
            "print('Hello, x=',x,' y=',y,'\n');";
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
#ifndef PROD
    cout << expr << endl;
    cout << "-------------" << endl;
#endif
    Status* translateStatus = translator->translate(expr, &code);
    if (translateStatus != NULL && translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
                "error '%s'\n",
                line, offset,
                translateStatus->getError().c_str());
    } else {

        assert(code != 0);
        vector<Var*> vars;

        if (isDefaultExpr) {
            Var* xVar = new Var(VT_DOUBLE, "x");
            Var* yVar = new Var(VT_DOUBLE, "y");
            vars.push_back(xVar);
            vars.push_back(yVar);
            xVar->setDoubleValue(42.0);
        }
#ifndef PROD
        code->disassemble();

        cout << "-------" << endl;
#endif
        Status* execStatus = code->execute(vars);
        if (execStatus != NULL && execStatus->isError()) {
            printf("Cannot execute expression: error: %s\n",
                    execStatus->getError().c_str());
        } else {
            if (isDefaultExpr) {
                printf("x evaluated to %f\n", vars[0]->getDoubleValue());
                for (uint32_t i = 0; i < vars.size(); i++) {
                    delete vars[i];
                }
            }
        }
        if (code != NULL) delete code;
        if (execStatus != NULL) delete execStatus;
    }
    if (translateStatus != NULL) delete translateStatus;
    if (translator != NULL) delete translator;

    if (!isDefaultExpr) {
        delete [] expr;
    }

    return 0;

}

