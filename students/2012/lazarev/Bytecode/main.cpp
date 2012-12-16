#include "parser.h"


int main(int argc, char **argv)
{
	char* text_buffer = loadFile(argv[1]);
	if(!text_buffer) {
		std::cout << "Can't load file: " << argv[1] << endl;
		return 1;
	}
	std::string text(text_buffer);

	Parser parser;
	Status *pStatus = parser.parseProgram(text);
	if(pStatus && pStatus->isError()) {
		std::cout << pStatus->getError();
		return 1;
	}

	parser.top()->node()->body()->visit(new AstPrinter());
}
