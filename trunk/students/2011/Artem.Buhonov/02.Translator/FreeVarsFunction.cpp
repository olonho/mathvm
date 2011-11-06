#include "FreeVarsFunction.h"

using namespace std;
using namespace mathvm;

FreeVarsFunction::FreeVarsFunction( mathvm::AstFunction *func ) : TranslatedFunction(func)
{	
	VarsSearcherVisitor fvv;
	func->node()->visit(&fvv);
	_freeVars = fvv.freeVars();
	setLocalsNumber(fvv.localsCount());
}


FreeVarsFunction::~FreeVarsFunction()
{
}

std::vector<const mathvm::AstVar *> & FreeVarsFunction::freeVars() 
{
	return _freeVars;
}