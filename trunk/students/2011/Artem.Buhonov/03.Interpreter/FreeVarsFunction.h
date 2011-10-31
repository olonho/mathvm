#pragma once
#include "mathvm.h"
#include "VarsSearcherVisitor.h"
#include "NewByteCode.h"
#include <vector>
#include <string>
#include <ostream>

class FreeVarsFunction : public mathvm::TranslatedFunction
{
public:
	FreeVarsFunction(mathvm::AstFunction *func);
	~FreeVarsFunction();
	NewByteCode *bytecode() {
		return &_bytecode;
	}
	virtual void disassemble(std::ostream& out) const {
		_bytecode.dump(out);
	}
	std::vector<const mathvm::AstVar *>& freeVars();
private:
	std::vector<const mathvm::AstVar *> _freeVars;
	NewByteCode _bytecode;
};