
#ifndef EXECUTABLECODE_H_
#define EXECUTABLECODE_H_

#include "mathvm.h"
#include <stack>

namespace mathvm {

union unit{
	uint64_t intVal;
	double doubleVal;
	uint16_t strValId;
};


class ExecutableCode: public Code {
public:
	ExecutableCode();
	virtual ~ExecutableCode();

	virtual Status* execute(vector<Var*> &vars);

private:
	std::stack<unit> mStack;
	Status* execute(Bytecode *bytecode);
};

}

#endif /* EXECUTABLECODE_H_ */
