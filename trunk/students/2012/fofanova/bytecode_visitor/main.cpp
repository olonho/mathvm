#include <iostream>
#include <string>
#include "parser.h"
#include "ast.h"
#include "bytecode_visitor.h"

using namespace mathvm;
using namespace std;

class MyCode: public Code {
        public:
        Status* execute(vector<Var*> & vars) {
                return 0;
        }
};

int main(int argc, const char * argv[])
{    
    char* text_buffer = loadFile(argv[1]);
    if(!text_buffer) {
    	cout << "Can't load file: " << argv[1] << endl;
    	return 1; 
    }
    const std::string text(text_buffer);
    
    Parser parser;
    Status *pStatus = parser.parseProgram(text);
    if(pStatus && pStatus->isError()) {
        cout << pStatus->getError();
        return 1;
    }
		Code* code = new MyCode();
		BytecodeFunction * main = new BytecodeFunction(parser.top());
		code->addFunction(main);
		parser.top()->node()->visit(new ByteCodeVisitor(code));
		main->bytecode()->add(BC_STOP);
    code->disassemble(std::cout);
    return 0;
} 
