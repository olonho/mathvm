#include "mathvm.h"

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
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

    const char* expr = "function int intMethod(int i) native 'intMethod';"
            "function int func(int a, int b) {"
            "    int i;"
            "    i = 0;"
            "    for (i in 0 .. 10) {"
            "        print('i=', i);"
            "    }"
            "    return a * b;"
            "}"
            "function void f(int a) {"
            "    return;"
            "}"
            "int x;"
            "x = 7;"
            "print('x=', x);"
            "double y;"
            "y = 9.0;"
            "string s;"
            "s = 'Hello \r\"World\"\n'"
            "for (x in 0..10) {"
            "    print('x=', x);"
            "}"
            "while (x < 10) {"
            "    print('x=', x);"
            "}"
            "if (y < 11.0) {"
            "   y += 1.0;"
            "}"
            "if (y < 13.0) {"
            "   y += 1.0;"
            "} else {"
            "   y += 2.0;"
            "}";

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
