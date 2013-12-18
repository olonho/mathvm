#pragma once

#include "mathvm.h"

class InterpreterCodeImpl : public mathvm::Code
{
public:
	InterpreterCodeImpl() {}
	virtual ~InterpreterCodeImpl() {}

	virtual mathvm::Status* execute(std::vector<mathvm::Var*>& vars);
};
