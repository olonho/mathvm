#include "mathvm.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

#include "bcinterpreter.h"
#include "my_utils.h"

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    string impl = "interpreter";
    const char* script = NULL;
    for (int32_t i = 1; i < argc; i++) {
        DEBUG_MSG(argv[i]);
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

    const char* expr =
            "function int a() {"
            "   return 11;"
            "}"
            "print(a());";
    bool isDefaultExpr = true;

//    script = "/home/mrx/Svn/MathVm/mathvm/students/2014/habibulin/1/mytests/vars.mvm";
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
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
               "error '%s'\n",
               line, offset,
               translateStatus->getError().c_str());
    } else {
        BcInterpreter interpreter;
        try {
            interpreter.interpret(code);
            delete code;
        } catch (exception const& e) {
            cout << "interpretation error:\n" << e.what() << endl;
            delete code;
        }

//        assert(code != 0);
//        vector<Var*> vars;

//        if (isDefaultExpr) {
//            Var* xVar = new Var(VT_DOUBLE, "x");
//            Var* yVar = new Var(VT_DOUBLE, "y");
//            vars.push_back(xVar);
//            vars.push_back(yVar);
//            xVar->setDoubleValue(42.0);
//        }
//        Status* execStatus = code->execute(vars);
//        if (execStatus->isError()) {
//            printf("Cannot execute expression: error: %s\n",
//                   execStatus->getError().c_str());
//        } else {
//            if (isDefaultExpr) {
//              printf("x evaluated to %f\n", vars[0]->getDoubleValue());
//              for (uint32_t i = 0; i < vars.size(); i++) {
//                delete vars[i];
//              }
//            }
//        }
//        delete code;
//        delete execStatus;
    }
    delete translateStatus;
    delete translator;

    if (!isDefaultExpr) {
      delete [] expr;
    }

    return 0;
}
