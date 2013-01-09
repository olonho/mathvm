#include "parser.h"
#include "BytecodeBuilder.h"
#include "BytecodeImpl.h"
#include <iostream>

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    string impl = "";
    const char* script = 0;
    if (argc < 2) {
        cerr << "Provide source code file as first parameter." << endl;
        return 1;
    } else {
        script = argv[1]; 
    }

    bool showDisassemble = argc > 2;

    const char* expr = loadFile(script);
    if (expr == 0) {
        cerr << "Cannot read file:" << script << endl;
        return 1;
    }

    BytecodeBuilder builder;
    BytecodeImpl* code = 0;
    Status* buildStatus = builder.translate(string(expr), (Code**) &code);

    if(buildStatus != 0 && buildStatus->isError()) {
        cerr << "Error in building source." << endl;
        delete buildStatus;
        return 1;
    }

    if (showDisassemble) {
    	code->disassemble();
    	cout << "Looks good. Executing..." << endl;
    }
    std::vector<Var*> vars;
    code->execute(vars);

    delete [] expr;
    return 0;
}
