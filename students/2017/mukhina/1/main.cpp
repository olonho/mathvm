#include "mathvm.h"

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    string impl = "printer";
    Translator* printer = Translator::create(impl);
    string program = "function int func(int a, int b) {"
                    "    int i;"
                    "    i = 0;"
                    "    for (i in 0 .. 10) {"
                    "        print('i=', i);"
                    "    }"
                    "    return a * b;"
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

    Code* code;
    printer->translate(program, &code);

    delete printer;
    return 0;
}
