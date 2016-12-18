#include "mathvm.h"
#include "translator_impl.h"

using namespace mathvm;
using namespace std;

namespace {
    void print_help() {
        cout << "\tUsage: mvm [<options>] <source file>\n";
        cout << "\tAvailable options: \n";
        cout << "\t-p\tonly print result (no execution); if no other options passed, just reformat source\n";
        cout << "\t-b\tproduce bytecode; execute it unless -p is passed\n";
        cout << "\t-j\tproduce bytecode; execute it with jit unless -p is passed\n";
    }
}

int main(int argc, char** argv) {
    string impl = "";
    const char* script = NULL;
    bool print_only = false;
    impl = TR_BYTECODE;

    for (int32_t i = 1; i < argc; i++) {
        if (string(argv[i]) == "-j") {
            impl = TR_JITCODE;
        } else if (string(argv[i]) == "-p") {
            print_only = true;
        } else if (string(argv[i]) == "-b") {
            impl = TR_BYTECODE;
        } else if (string(argv[i]) == "-h") {
            print_help();
            return 0;
        }
        else {
            script = argv[i];
        }
    }

    if (impl == TR_FORMATTER) {
        print_only = true;
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
        if (print_only) {
            if (impl != TR_FORMATTER) {
                code->disassemble(cout);
            }
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
                       execStatus->getErrorCstr());
            } else {
                if (isDefaultExpr) {
                    printf("x evaluated to %f\n", vars[0]->getDoubleValue());
                    for (uint32_t i = 0; i < vars.size(); i++) {
                        delete vars[i];
                    }
                }
            }
            delete execStatus;
        }
        delete code;
    }
    delete translateStatus;
    delete translator;

    if (!isDefaultExpr) {
        delete [] expr;
    }

    return 0;
}
