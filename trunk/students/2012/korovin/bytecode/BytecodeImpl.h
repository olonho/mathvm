/*
 * BytecodeImpl.h
 *
 *  Created on: 24.10.2012
 *      Author: temp1ar
 */

#ifndef BYTECODEIMPL_H_
#define BYTECODEIMPL_H_

#include "mathvm.h"

namespace mathvm {

class BytecodeImpl: public Code {
public:
	BytecodeImpl();
	virtual ~BytecodeImpl();

	virtual Status* execute(vector<Var*>& vars);
};

} /* namespace mathvm */
#endif /* BYTECODEIMPL_H_ */
