#include "mathvm.h"

#include <stdio.h>

#include <iostream> 

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    Translator* translator = Translator::create();

    if (translator == 0) {
        cout << "TODO: Implement translator factory in translator.cpp!!!!" << endl;
        return 1;
    }

    const char* expr = "double x; double y;"
                        "x += 8.0; y = 2.0;" 
                        "print('Hello, x=',x,' y=',y,'\n');"
        ;
    bool isDefaultExpr = true;

    if (argc > 1) {
        expr = loadFile(argv[1]);
        if (expr == 0) {
            printf("Cannot read file: %s\n", argv[1]);
            return 1;
        }
        isDefaultExpr = false;
    }

    Code* code = 0;

    Status* translateStatus = translator->translate(expr, &code);
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
               "error '%s'\n",
               line, offset,
               translateStatus->getError().c_str());
#if 0
        if (position != Status::INVALID_POSITION) {
            printf("%s\n", expr);
            for (uint32_t i = 0; i < position; i++) {
                printf(" ");
            }
            printf("^\n");
        }
#endif
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
        Status* execStatus = code->execute(vars);
        if (execStatus->isError()) {
            printf("Cannot execute expression: error: %s\n",
                   execStatus->getError().c_str());
        } else {
            if (isDefaultExpr) {
                printf("x evaluated to %f\n", vars[0]->getDoubleValue());
            }
        }
        delete code;
        delete execStatus;
    }
    delete translateStatus;
    delete translator;

    return 0;
}
