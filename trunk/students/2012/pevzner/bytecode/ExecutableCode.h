
#ifndef EXECUTABLECODE_H_
#define EXECUTABLECODE_H_

#include "mathvm.h"

namespace mathvm {

union unit{
	uint64_t intVal;
	double doubleVal;
	uint16_t idVal;
};

class ExecutableCode: public Code {
public:
	ExecutableCode();
	virtual ~ExecutableCode();

	virtual Status* execute(vector<Var*>& vars);
};

}

#endif /* EXECUTABLECODE_H_ */
