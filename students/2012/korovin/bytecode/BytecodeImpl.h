/*
 * BytecodeImpl.h
 *
 *  Created on: 24.10.2012
 *      Author: temp1ar
 */

#ifndef BYTECODEIMPL_H_
#define BYTECODEIMPL_H_

#include "mathvm.h"
#include <AsmJit/Assembler.h>

namespace mathvm {

using namespace AsmJit;

union value {
    int64_t i;
    double d;
    const char* sPtr;
};

class Context {
	Context* parent_;
	const BytecodeFunction* function_;
	vector<value> storage_;

	Context* contextById(int contextId) {
		if (function_->id() == contextId)
			return this;
		else {
			assert(parent_ != 0);
			return parent_->contextById(contextId);
		}
	}
public:
	Context(Context* parent, const BytecodeFunction* function)
		: parent_(parent)
		, function_ (function)
		, storage_(vector<value>())
	{
        storage_.resize(function_->parametersNumber() + function_->localsNumber());
    }

	vector<value>& vars() {
		return storage_;
	}

	vector<value>& vars(int contextId) {
		return contextById(contextId)->storage_;
	}
};

class BytecodeImpl: public Code {
    vector<value> stack_;
    Context* context_;

public:
	BytecodeImpl();
	virtual ~BytecodeImpl();
	virtual Status* execute(vector<Var*>& vars);
	void executeFunction(BytecodeFunction* f);
};

} /* namespace mathvm */
#endif /* BYTECODEIMPL_H_ */
