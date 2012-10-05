#include <iostream>

#include "mathvm.h"
#include "main.h"
#include "printastvisitor.h"

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {

    const char* programSourceFile = "function.mvm";
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

























