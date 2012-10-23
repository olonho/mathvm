#ifndef __BYTECODE_IMPL_H__
#define __BYTECODE_IMPL_H__

#include "mathvm.h"

#include "interpreterimpl.h"

using namespace mathvm;

class BytecodeImpl : public Code
{
public:
	virtual Status* execute(vector<Var*>& vars)
	{
		InterpreterImpl runner(this);
		runner.run();
		return 0;
	}
};

#endif /* __BYTECODE_IMPL_H__ */
