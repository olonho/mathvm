#include "mathvm.h"
#include "parser.h"
#include "SourceByASTPrinter.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

using namespace mathvm;


int main(int argc, char** argv) {

    if(argc < 2) {
    	std::cout<<"No source file name param!";
    	return 1;
    }

    const char* expr = loadFile(argv[1]);
    if (expr == 0) {
         printf("Cannot read file: %s\n", argv[1]);
         return 1;
    }

	Parser parser;
    Status *status = parser.parseProgram(expr);
    if(status != 0){
    	std::cout<<"Cannot parse source : expression at %d,%d; "
                   "error '%s'\n",
                   status->getError().c_str();
    	delete status;
    	return 1;
    }

    SourceByASTPrinter printer;
    printer.performPrint(parser.top());

    return 0;
}

/*int main(int argc, char** argv) {
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

    Parser parser;
    parser.parseProgram(expr);

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
}*/
