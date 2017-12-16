#pragma once

#include "ast.h"
#include "bytecode.h"

#include <unordered_map>

namespace mathvm { namespace ldvsoft {

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
	unordered_map<uint16_t, NativeFunctionDescriptor> natives;

	BytecodeCode() = default;
	virtual ~BytecodeCode() override = default;

 	virtual Status *execute(vector<Var*> &vars) override;
    virtual void disassemble(ostream &out, FunctionFilter *filter) override;
	uint16_t makeNativeFunction(string const &name, Signature const &sign, void *addr);
	NativeFunctionDescriptor const &nativeById(uint16_t id) const;
};

}}
