#include "parser.h"
#include "ExecutableCode.h"
#include "ASTtoByteCodeTranslator.h"
#include <iostream>

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
	if(argc < 2) {
		std::cout<<"No source file name param!";
		return 1;
	}

	const char* source = loadFile(argv[1]);
	if (source == 0) {
		std::cout<<"Cannot read file: "<<argv[1]<<std::endl;
		return 1;
	}

	Parser parser;
	Status *status = parser.parseProgram(source);
	if(status != 0){
		std::cout<<"Cannot parse source : error '"<<status->getError().c_str()<<"'"<<std::endl;
		delete status;
		return 1;
	}


	ExecutableCode *code = new ExecutableCode();
	ASTtoByteCodeTranslator translator(code);
	translator.performTranslation(parser.top());

    //code->disassemble();

    std::vector<Var*> v;
    code->execute(v);

    delete source;

    return 0;
}
