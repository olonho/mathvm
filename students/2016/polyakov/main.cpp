#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>
#include <fstream>

#include "translator.h"
#include "mathvm.h"
#include "jit.h"

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Not enough arguments - provide paths to file with mvm code" << endl;
        return 0;
    }

    const char* script = "";
    const char* outFile = "";

    string impl = "";
    for (int32_t i = 1; i < argc; i++) {
      if (string(argv[i]) == "-j") {
        impl = "jit";
      } if (string(argv[i]) == "-p") {
        impl = "printer";
      } if (string(argv[i]) == "-t") {
        impl = "translate";
      } else {
        if (string(script) == "") {
          script = argv[i];
        } else {
          outFile = argv[i];
        }
      }
    }
    Translator* translator = Translator::create(impl);

    if (translator == 0) {
        cout << "TODO: Implement translator factory in translator.cpp!!!!" << endl;
        return 1;
    }

    bool isDefaultExpr = true;

    const char* expr = 0;

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
      if (impl == "translate") {
        assert(code != 0);
        ostream* out = &cout;

        ofstream outputfs(outFile);
        if (string(outFile) != "") {
            if (outputfs.fail()) {
                cout << "Can't not write to file '" << outFile << "'" << endl;
            } else {
                out = &outputfs;
            }
        }

        code->disassemble(*out);
        outputfs.close();
      }
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
