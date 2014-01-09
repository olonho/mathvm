/*
 * Translator.h
 *
 *  Created on: Dec 8, 2013
 *      Author: sam
 */

#ifndef BCINTERPRITATOR_H_
#define BCINTERPRITATOR_H_

#include <vector>
#include "mathvm.h"

namespace mathvm {

class BCInterpritator: public Code {
public:
	void exec() {
	}

	BCInterpritator() {
	}

	~BCInterpritator() {
	}
	Status* execute(std::vector<Var*>& vars) {
		disassemble();
		return 0;
	}
};

} /* namespace mathvm */
#endif /* BCINTERPRITATOR_H_ */
