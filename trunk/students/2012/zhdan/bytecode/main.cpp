#include <iostream>
#include <string>
#include <sstream>
#include "parser.h"
#include "ast.h"
#include "bytecodegen.h"
#include "typechecker.h"
#include "interpreter.h"

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
	TypeCheckerVisitor checker;
	checker.calculateTypes(parser.top());
//	cout << "checked" << '\n';
	Code* c = new MyCode();
	CodeGenVisitor v(c);	
	BytecodeFunction* f = v.generate(parser.top());
//	cout << "generated" << '\n';
	Interpreter i(c);
//	f->disassemble(cout);
	i.execute(f);
    return 0;
} 
