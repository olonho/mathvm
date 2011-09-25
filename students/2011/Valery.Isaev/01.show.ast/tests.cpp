#include <sstream> 
#include <stdio.h> 

#include "ShowVisitor.h"
#include "CompareVisitor.h"
#include "parse.cpp"

const char* tests[] =
    { "../../../../tests/add.mvm"
    , "../../../../tests/div.mvm"
    , "../../../../tests/for.mvm"
    , "../../../../tests/if.mvm"
    , "../../../../tests/literal.mvm"
    , "../../../../tests/mul.mvm"
    , "../../../../tests/sub.mvm"
    , "../../../../tests/var.mvm"
    , "../../../../tests/while.mvm"
    };

int main() {
    int passed = 0, failed = 0;
    for (unsigned int i = 0; i < sizeof(tests) / sizeof(char*); ++i) {
        mathvm::Parser p, p2;
        parseFile(p, tests[i]);
        std::stringstream stream;
        ShowVisitor v(stream);
        v.show(p.top());
        parseExpr(p2, stream.str());
        CompareVisitor cv;
        bool res = cv.compare(p.top(), p2.top());
        if (!res) {
            fprintf(stderr, "%s: FAILURE\n%s\n", tests[i], stream.str().c_str());
            ++failed;
        } else {
            ++passed;
        }
    }
    printf("%d passed, %d failed, %d total\n", passed, failed, passed + failed);
    return 0;
}
