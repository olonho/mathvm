//
// Created by austud on 10/12/16.
//

#include <iostream>
#include <string>

#include "transformer.h"
#include "../../../vm/parser.h"

using namespace std;
using namespace mathvm;

int main(int argc, char** argv) {
    string code = "";
    if (argc != 2) {
        cout << "specify input file name\n";
        return 1;
    }
    code = string(loadFile(argv[1]));

    Parser parser;
    Status *s = parser.parseProgram(code);
    if (!s->isOk()) {
        cout << "parse error\n";
        return 1;
    }

    transformer tr(true);

    parser.top()->node()->body()->visit(&tr);

    cout << tr.to_str() << endl;

    return 0;
}
