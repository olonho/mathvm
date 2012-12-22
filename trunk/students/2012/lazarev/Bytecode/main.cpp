#include "parser.h"
#include "BytecodeGenerationVisitor.h"
#include "InterpreterCodeImpl.h"

using namespace mathvm;

int main(int argc, char **argv)
{
	char* text_buffer = loadFile(argv[1]);
	if(!text_buffer) {
		std::cout << "Can't load file: " << argv[1] << std::endl;
		return 1;
	}
	std::string text(text_buffer);

	Parser parser;
	Status *pStatus = parser.parseProgram(text);
	if(pStatus && pStatus->isError()) {
		std::cout << pStatus->getError();
		return 1;
	}

	InterpreterCodeImpl *code = new InterpreterCodeImpl();
	parser.top()->node()->visit(new BytecodeGenerationVisitor(code));
	vector<Var*> v;
	code->execute(v);
}
