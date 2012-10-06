#include <iostream>

#include "mathvm.h"
#include "main.h"
#include "printastvisitor.h"

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
    
    if(argc < 2) {
        cout << "Need *.mvm file" << endl;
        return 3;
    }
    
    const char* programSourceFile = argv[1];
    const char* expr = loadFile(programSourceFile);
    
    if (expr == 0) {
        cerr << "Cannot read file: " << programSourceFile << endl;
        return 1;
    }
    
    Parser parser;
    Status* status = parser.parseProgram(expr);
    if(status != NULL && status->isError()) {
        uint32_t position = status->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        cerr << "Cannot translate expression: expression at "
             << line << ", " << offset << "; " << endl
             << "error " 
             << status->getError().c_str() << endl;
        delete[] expr;
        return 2;    
    }
    
    showFormatedCode(parser.top());

    delete[] expr;
    
    return 0;
    
}

void showFormatedCode(const AstFunction* rootFunction){
    
    PrintAstVisitor printVisitor;
    
    printVisitor.visitTopFunction(rootFunction);

}

























