
#include <iostream>
#include <cstdio>

#include <fcntl.h>
#include <sys/stat.h>

#include "mathvm.h"
#include "bci.h"
#include "bda.h"


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

//    const char* expr = "double x; double y;"
//                        "x += 8.0; y = 2.0;"
//                        //"print('Hello, x=',x,' y=',y,'\n');"
//                        "int i;"
//                        "for(i in 1 .. 10)"
//                        "{ if ((i % 2) == 0) "
//                        "  { int z = 0; print(i, '\n'); } }";

    bool isDefaultExpr = true;
  
    const char * expr;

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
      std::cerr << translateStatus->getError();
      delete translateStatus;
      return -1;
    }
  
    //std::cout << disassemble(code).str();
    code->disassemble(std::cout);
  
    std::cout << "Execution:\n";
  
    BytecodeInterpreter::exec(static_cast<BytecodeExecutable *>(code));
  
    return 0;
  
    if (translateStatus->isError()) {
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
        Status* execStatus = code->execute(vars);
        if (execStatus->isError()) {
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
        delete code;
        delete execStatus;
    }
    delete translateStatus;
    delete translator;

    if (!isDefaultExpr) {
      delete [] expr;
    }

    return 0;
}
