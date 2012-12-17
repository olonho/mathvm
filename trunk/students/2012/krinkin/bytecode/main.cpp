#include "mathvm.h"
#include "parser.h"

#include "bctranslator.h"
#include "bccompiler.h"
#include "typer.h"

#include <iostream>
#include <vector>
#include <memory>

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
	BCTransaltor translator;
    std::auto_ptr<Status> status(translator.translate(expr, &code));
	if (status.get())
	{
		std::cerr << "Error(" << status->getPosition() << "): " << status->getError() << std::endl;
		return 3;
	}
	
//	std::vector<Var*> vars;
//    code->execute(vars);
    
    BCCompiler compiler((BCCode *)code);
    compiler.execute();

    delete code;
	delete [] expr;

	return 0;
}
