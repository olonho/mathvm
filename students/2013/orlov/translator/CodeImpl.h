#ifndef CODEIMPL_H_
#define CODEIMPL_H_

#include <mathvm.h>
#include <ast.h>
#include <visitors.h>
#include <parser.h>
#include <stack>

namespace mathvm {

class CodeImpl: public Code {
	class FuncScope {
	public:
		FuncScope(BytecodeFunction * f, FuncScope * parent = 0): ip(0), parent(parent), function(f)
		{
			size_t numVars = f->localsNumber() + f->parametersNumber();
			for (size_t i = 0; i < numVars; ++i) {
				vars.push_back(Var(VT_INT, ""));
			}
		}
		uint32_t ip;
		FuncScope * parent;
		BytecodeFunction * function;
		std::vector<Var> vars;
	};

public:
	CodeImpl(): Code () {}
	Status* execute(vector<Var*>& vars);

private:
	void exec_dload(double val);
	void exec_iload(int64_t val);
	void exec_sload(uint16_t id);
	void doubleBinOp(TokenKind op);
	void intBinOp   (TokenKind op);
	void loadVar (uint16_t id);
	void storeVar(uint16_t id);
	void loadVar (uint16_t context_id, uint16_t id);
	void storeVar(uint16_t context_id, uint16_t id);
	Var popStack();
	void callFunction(uint16_t id);
	void callNative(uint16_t id);

	template<class T>
	T getVal(uint16_t shift) {
		T v = currentScope->function->bytecode()->getTyped<T>(currentScope->ip);
		currentScope->ip += shift;
		return v;
	}

	FuncScope * currentScope;
	std::stack<Var> varStack;
};

}
#endif /* CODEIMPL_H_ */
