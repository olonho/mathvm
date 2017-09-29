#include <iostream>

#include "../include/mathvm.h"

using namespace mathvm;
using namespace std;

int main() {
    Translator* translator = Translator::create("printer");

    const char* expr = "function int add(int x, int y) {"
                        "    return x + y;"
                        "}"
                        "function void doit() {"
                        "    print('Hi\n');"
                        "}"
                        "function double mul5(int max, double x1, double x2, double x3, double x4, double x5) {"
                        "    double r;"
                        "    r = 1.0;"
                        "    if (max > 1) {"
                        "        r = r * x1;"
                        "    }"
                        "    if (max > 2) {"
                        "        r = r * x2;"
                        "    }"
                        "    if (max > 3) {"
                        "        r = r * x3;"
                        "    }"
                        "    if (max > 4) {"
                        "        r = r * x4;"
                        "    }"
                        "    if (max > 5) {"
                        "        r = r * x5;"
                        "    }"
                        "    return r;"
                        "}"
                        "function int fact(int n) {"
                        "    if (n < 3) {"
                        "        return n;"
                        "    }"
                        "    return n*fact(n-1);"
                        "}"
                        "double x; double y;"
                        "x += 8.0; y = 2.0;"
                        "print('Hello, x=',x,' y=',y,'\n');"
                        "int i;"
                        "int j;"
                        "j = 3;"
                        "print ('Begin.\n');"
                        "for (i in -3..4)  {"
                        "    j += i; print(i, ' ');"
                        "}"
                        "for (i in 1..2)  {"
                        "    j += i;"
                        "}"
                        "while (1) {"
                        "    print(i);"
                        "}"
                        "print('\n', j, '\n');"
                        "print ('The end.\n');"
                        "function double pow(double x, double y) native \'pow\';"
                        "function void foo(int a, double b, string c) {"
                        "    function string g() {"
                        "        print('abc');"
                        "    }"
                        "    foo(1, 1.0, 'asd');"
                        "    print('xyz');"
                        "}"
                        "x = (1 | 5);"
                        "x = (1 & 6);"
                        "x = (1 && 4);"
                        "x = (1 ^ 5);"
                        "x = (1 == 5);"
                        "x = (1 != 5);"
                        "x = (1 >= 5);"
                        "x = (1 > 5);"
                        "x = (1 <= 5);"
                        "x = (1 < 5);"
                        "x = (1 % 5);"
                        "x = (1 * 5);"
                        "x = (1 / 5);"
                        "x = (1 + 5);"
                        "x = (1 - 5);"
                        "x = 7;"
                        "y = 8;"
                        "if (x < y && y > 1 && !(x < 2)) {"
                        "    print('1: Less\n');"
                        "}"
                        "if (x == y) {"
                        "    print('2: Equal\n');"
                        "} else {"
                        "    print('2: Different\n');"
                        "}"
                        "if (x <= 7) {"
                        "    print('3: Lesser\n');"
                        "} else {"
                        "    print('3: Greater\n');"
                        "}"
                        "if (x >= 77) {"
                        "    print('4: Greater\n');"
                        "} else {"
                        "    print('4: Lesser\n');"
                        "}";

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
        delete code;
    }

    delete translator;
    delete translateStatus;

    return 0;
}
