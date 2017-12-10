#pragma once

#include "ast.h"
#include "bytecode.h"

#include <unordered_map>

namespace mathvm::ldvsoft {

class BytecodeCode: public mathvm::Code {
private:
	class Evaluator;
public:
	class TranslatedFunction : public mathvm::BytecodeFunction {
	public:
		AstFunction *function;
		vector<size_t> scopes;

		TranslatedFunction(AstFunction *function):
			mathvm::BytecodeFunction(function), function{function} {}
		virtual ~TranslatedFunction() override = default;
	};

	unordered_map<uint16_t, unordered_map<string, uint16_t>> scopes;

	BytecodeCode() = default;
	virtual ~BytecodeCode() override = default;

 	virtual Status *execute(vector<Var*> &vars) override;
    virtual void disassemble(ostream &out, FunctionFilter *filter) override;
};

}
