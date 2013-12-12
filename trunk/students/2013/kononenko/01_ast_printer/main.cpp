#include "stdafx.h"

#include "parser.h"
#include "mathvm.h"
#include "ast_printer.h"

int main(int argc, char* argv[]) 
{
    if(argc < 2) 
    {
        cerr << "Usage: ast_printer filename" << endl;
        return 1;
    } 

    char *buffer =  mathvm::loadFile(argv[1]);
    const string text(buffer);
    delete[] buffer;

    mathvm::Parser parser;
    mathvm::Status* status = parser.parseProgram(text);

    if(status != NULL && !status->isOk()) 
    {
        cerr << status->getError() << endl;
        return 1;       
    }

    mathvm::ast_printer printer;
    cout << printer.print_tree(parser.top()->node());

    return 0;
}