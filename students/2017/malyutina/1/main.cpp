#include "../../../../include/mathvm.h"
#include "ast_printer.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

using namespace mathvm;
using namespace std;

int main(int argc, char **argv) {
    string impl = "";
    const char *script = NULL;
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
//    cout << " task = " << impl << endl;
    Translator *translator = Translator::create(impl);

    if (translator == 0) {
        cout << "TODO: Implement translator factory in translator.cpp!!!!" << endl;
        return 1;
    }

    const char *expr =
            "int i;double d;i = 1;d = 2.0;"
                    "print('i-1=', i - 1,'\n');"
                    "print('d-1=', d-1.0,'\n');"
                    "print('i-123456789012345=', i-123456789012345,'\n');"
                    "print ('d-1e9=',d-1e9,'\n');"
                    "function void top() { return;}";

    bool isDefaultExpr = true;

    if (script != NULL) {
        expr = loadFile(script);
        if (expr == 0) {
            printf("Cannot read file: %s\n", script);
            return 1;
        }
        isDefaultExpr = false;
    }

    Code *code = 0;
    Status *translateStatus = translator->translate(expr, &code);
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
                       "error '%s'\n",
               line, offset,
               translateStatus->getErrorCstr());
    }

//    else {
//        if (impl != "printer") {
//            assert(code != 0);
//            vector < Var * > vars;
//
//            if (isDefaultExpr) {
//                Var *xVar = new Var(VT_DOUBLE, "x");
//                Var *yVar = new Var(VT_DOUBLE, "y");
//                vars.push_back(xVar);
//                vars.push_back(yVar);
//                xVar->setDoubleValue(42.0);
//            }
//            Status *execStatus = code->execute(vars);
//            if (execStatus->isError()) {
//                printf("Cannot execute expression: error: %s\n",
//                       execStatus->getErrorCstr());
//            } else {
//                if (isDefaultExpr) {
//                    printf("x evaluated to %f\n", vars[0]->getDoubleValue());
//                    for (uint32_t i = 0; i < vars.size(); i++) {
//                        delete vars[i];
//                    }
//                }
//            }
//            delete code;
//            delete execStatus;
//        }
//    }

    delete translateStatus;
    delete translator;
    delete code;

    if (!isDefaultExpr) {
        delete[] expr;
    }

    return 0;
}
