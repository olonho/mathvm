#pragma once

#include "mathvm.h"
#include "bytecode.h"

namespace mathvm::ldvsoft {

class BytecodeCode: public mathvm::Code {
public:
	Bytecode bytecode;
 
 	virtual Status *execute(vector<Var*> &vars) override;
};

}
