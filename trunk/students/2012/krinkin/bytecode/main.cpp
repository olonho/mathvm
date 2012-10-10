#include "parser.h"
#include "typechecker.h"

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
	Parser parser;
	/*
	 * parseProgram return 0 if it success, but it is weird until
	 *                            mathvm::Status contains isError and isOk
	 */
	std::auto_ptr<Status> parse_status(parser.parseProgram(expr));
	delete [] expr;
	if (parse_status.get())
	{
		std::cerr << "Error(" << parse_status->getPosition() << "): "
		          << parse_status->getError() << std::endl;
		return 3;
	}

	TypeChecker checker;

	std::auto_ptr<Status> check_status(checker.check(parser.top()));
	if (check_status.get())
	{
		std::cerr << "Error(" << check_status->getPosition() << "): "
		          << check_status->getError() << std::endl;
		return 4;
	}

	return 0;
}
