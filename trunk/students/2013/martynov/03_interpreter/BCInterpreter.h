#ifndef BCINTERPRETER_H_
#define BCINTERPRETER_H_

#include "mathvm.h"
#include "BCTypes.h"

namespace mathvm {

class BCInterpreter: public Code {
public:
	BCInterpreter();
	~BCInterpreter();
	std::map<std::string, uint16_t>& globalVarsMap() {
		return _globalVarsMap;
	}
	Status* execute(std::vector<Var*>& vars);
private:
	Value* Stack;
	StackEntry* CallStack;
	std::map<std::string, uint16_t> _globalVarsMap;
	void exec();
};

} /* namespace mathvm */

#endif
