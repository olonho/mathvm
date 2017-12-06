#pragma once

#include "mathvm.h"
#include "bytecode.h"

namespace mathvm::ldvsoft {

class BytecodeCode: public mathvm::Code {
public:
	class TranslatedFunction : public mathvm::BytecodeFunction {
	public:
		AstFunction *function;

		TranslatedFunction(AstFunction *function):
			mathvm::BytecodeFunction(function), function(function) {}
		virtual ~TranslatedFunction() override = default;
	};

	Bytecode bytecode;
 
	BytecodeCode() = default;
	virtual ~BytecodeCode() override = default;

 	virtual Status *execute(vector<Var*> &vars) override;
};

}
