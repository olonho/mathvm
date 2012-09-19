#include <iostream>
#include <memory>

#include "mathvm.h"
#include "parser.h"
#include "pretty.h"

using namespace mathvm;

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		std::cerr << "Usage:\n\tastprinter source.mvm" << std::endl
		          << "\tsource.mvm - name of source file" << std::endl;
		return 1;
	}
	
	/*
	 * Function mathvm::loadFile opens file, but doesn't close it,
	 *                                                    use it carefully!
	 */
	std::unique_ptr<char[]> expr(loadFile(argv[1]));
	if (!expr)
	{
		std::cerr << "Cannot read file " << argv[1] << std::endl;
		return 2;
	}
	
	Parser parser;
	/*
	 * parseProgram return 0 if it success, but it is weird until
	 *                            mathvm::Status contains isError and isOk
	 */
	std::unique_ptr<Status> status(parser.parseProgram(expr.get()));
	if (status && status->isError())
	{
		std::cerr << "Error(" << status->getPosition() << "): "
		          << status->getError() << std::endl;
		return 3;
	}

	PrettyPrinter printer(std::cout);
	printer.visitTopLevelBlock(parser.top());
	
	return 0;
}
