/*
 * ExecutableCode.h
 *
 *  Created on: Oct 25, 2012
 *      Author: alex
 */

#ifndef EXECUTABLECODE_H_
#define EXECUTABLECODE_H_

#include "mathvm.h"

namespace mathvm {

class ExecutableCode: public Code {
public:
	ExecutableCode();
	virtual ~ExecutableCode();
	virtual Status* execute(vector<Var*>& vars);
};

} /* namespace mathvm */
#endif /* EXECUTABLECODE_H_ */
