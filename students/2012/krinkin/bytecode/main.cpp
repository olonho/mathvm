#include "codegen.h"

using namespace mathvm;

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		std::cerr << "Usage:\n\tastprinter source.mvm" << std::endl
		          << "\tsource.mvm - name of source file" << std::endl;
		return 1;
	}
	
	char *expr = loadFile(argv[1]);
	if (!expr)
	{
		std::cerr << "Cannot open file " << argv[1] << std::endl;
		return 2;
	}
	
	Code *code;
	CodeGenerator generator;

	std::auto_ptr<Status> status(generator.translate(expr, &code));
	if (status.get())
	{
		std::cerr << "Error(" << status->getPosition() << "): "
		          << status->getError() << std::endl;
	}
	
	code->disassemble();
	
	delete code;
	delete [] expr;

	return 0;
}
