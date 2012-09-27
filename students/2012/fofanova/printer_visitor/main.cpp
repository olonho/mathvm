#include <iostream>
#include <string>
#include "parser.h"
#include "printer_visitor.h"

using namespace mathvm;
using namespace std;


int main(int argc, const char * argv[])
{    
    char* text_buffer = loadFile(argv[1]);
    if(!text_buffer) {
    	cout << "Can't load file: " << argv[1] << endl;
    	return 0; 
    }
    const std::string text(text_buffer);
    
    Parser parser;
    Status *pStatus = parser.parseProgram(text);
    if(pStatus && pStatus->isError()) {
        cout << pStatus->getError();
        return 0;
    }

    parser.top()->node()->visit(new PrintVisitor());
    return 0;
} 
