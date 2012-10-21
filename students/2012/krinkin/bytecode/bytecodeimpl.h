#ifndef __BYTECODE_IMPL_H__
#define __BYTECODE_IMPL_H__

#include "mathvm.h"

using namespace mathvm;

class BytecodeImpl : public Code
{
public:
	virtual Status* execute(vector<Var*>& vars)
	{
		return 0;
	}
};

#endif /* __BYTECODE_IMPL_H__ */
