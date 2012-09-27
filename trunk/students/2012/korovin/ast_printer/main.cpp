#include "AstPrinter.h"
#include <iostream>

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    string impl = "";
    const char* script = 0;
    if (argc < 2) {
        cerr << "Provide source code file as firts parameter." << endl;
        return 1;
    } else {
        script = argv[1]; 
    }

    const char* expr = loadFile(script);
    if (expr == 0) {
        cerr << "Cannot read file:" << script << endl;
        return 1;
    }
    
    Status* parseStatus;
    
    Parser parser;
    parseStatus = parser.parseProgram(expr);

    if(parseStatus != 0 && parseStatus->isError()) {
        cerr << "Error in parsing source." << endl;
        delete [] expr;
        delete parseStatus;
        return 1;
    }

    AstPrinter printer;
    printer.print(parser.top());

    delete [] expr;
    return 0;
}
