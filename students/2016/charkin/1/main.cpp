#include <iostream>
#include "mathvm.h"
#include "parser.h"
#include "printer.h"

using namespace std;
using namespace mathvm;

int main(int argc, char **argv)
{
    if (argc < 2) {
        cout << "need nvm filename" << endl;
        return -1;
    }

    const char * code = loadFile(argv[1]);
    if (code == 0) {
        cout << "wrong nvm filename" << endl;
        return -1;
    }

    Parser parser;
    Status * status = parser.parseProgram(code);
    if (status->isError()){
        cout << "cant parse programm" << endl;
    }
    Printer * printer = new Printer(cout);
    parser.top()->node()->visitChildren(printer);

    return 0;
}

