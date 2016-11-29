#include "mathvm.h"

#include <cstdio>
#include <cstddef>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

#include "mathvm_translator.h"
#include "mathvm_interpreter.h"

using namespace mathvm;

int main(int argc, char** argv) {
    string impl = "";
    const char* script = nullptr;

    for (int i = 1; i < argc; ++i)
    {
        if (string(argv[i]) == "-j")
            impl = "jit";
        else if (string(argv[i]) == "-t")
            impl = "translator";
        else
            script = argv[i];
    }

    Translator* translator = Translator::create(impl);

    const char* expr = "double x; double y;"
                        "x += 8.0; y = 2.0;"
                        "print('Hello, x=',x,' y=',y,'\n');";
    bool isDefaultExpr = true;

    if (script != nullptr)
    {
        expr = loadFile(script);
        if (expr == 0)
        {
            printf("Cannot read file: %s\n", script);
            return 1;
        }

        isDefaultExpr = false;
    }

    Code* code = new InterpreterCodeImpl();
    Status* translateStatus = translator->translate(expr, &code);

    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
               "error '%s'\n",
               line, offset,
               translateStatus->getErrorCstr());
    } else {
        if (impl != "printer") {
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
                   execStatus->getErrorCstr());
          } else {
            if (isDefaultExpr) {
              printf("x evaluated to %f\n", vars[0]->getDoubleValue());
              for (uint32_t i = 0; i < vars.size(); i++) {
                delete vars[i];
              }
            }
          }
          delete code;
          delete execStatus;
        }
    }
    delete translateStatus;
    delete translator;

    if (!isDefaultExpr) {
      delete [] expr;
  }

    return 0;
}
