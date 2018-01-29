#include "mathvm.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

using namespace mathvm;
using namespace std;

typedef long long int64;

int64 cnt = 0;

int64 fib(int x) {
    cnt++;
    if (x <= 1) {
        return x;
    }
    int64 r;
    r = fib(x - 1);
    return r + fib(x - 2);
}

int main(int argc, char** argv) {
//    cout << fib(35) << endl;
//    cout << cnt << endl;
//    return 0;
    string impl = "";
    const char* script = NULL;
    for (int32_t i = 1; i < argc; i++) {
      if (string(argv[i]) == "-j") {
        impl = "jit";
      }  if (string(argv[i]) == "-p") {
        impl = "printer";
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
          Status* execStatus = code->execute(vars);
          if (execStatus->isError()) {
              printf("Cannot execute expression: error: %s\n",
                     execStatus->getErrorCstr());
          }

          delete execStatus;
          delete code;
        }
    }
    delete translateStatus;
    delete translator;

    if (!isDefaultExpr) {
      delete [] expr;
    }

    return 0;
}
