#pragma once

#include <stack>
#include "mathvm.h"

using namespace std;
using namespace mathvm;

class CodeImpl : public Code
{
public:
	CodeImpl() {}
	virtual ~CodeImpl() {}

	virtual Status* execute(vector<Var*>& vars);

private:
	union Val {
		int64_t ival;
		double dval;
		uint16_t sval;
	};

	void initContexts();
	void executeBytecode(Bytecode* bc);

	bool stopped;

	stack<Val> compStack;
	vector<stack<vector<Val> > > contexts;
};
