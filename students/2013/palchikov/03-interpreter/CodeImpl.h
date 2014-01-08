#pragma once

#include "mathvm.h"

using namespace mathvm;

class CodeImpl : public Code
{
public:
	CodeImpl() {}
	virtual ~CodeImpl() {}

	virtual Status* execute(vector<Var*>& vars);
};
