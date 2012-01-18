#include "MyCode.h"
#include "BytecodeInterpreter.h"

using namespace mathvm;

MyCode::MyCode(void)
{
}

MyCode::~MyCode(void)
{
}

Status* MyCode::execute(std::vector<mathvm::Var*>& vars) {
	BytecodeInterpreter interpreter(this);
	return interpreter.call(0);
}
