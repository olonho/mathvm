#pragma once
#include "mathvm.h"
#include "VarsSearcherVisitor.h"
#include <vector>
#include <string>
#include <ostream>

class FreeVarsFunction : public mathvm::TranslatedFunction
{
public:
	FreeVarsFunction(mathvm::AstFunction *func);
	~FreeVarsFunction();
	mathvm::Bytecode *bytecode() {
		return &_bytecode;
	}
	virtual void disassemble(std::ostream& out) const {
		_bytecode.dump(out);
	}
	std::vector<const mathvm::AstVar *>& freeVars();
private:
	std::vector<const mathvm::AstVar *> _freeVars;
	mathvm::Bytecode _bytecode;
};
