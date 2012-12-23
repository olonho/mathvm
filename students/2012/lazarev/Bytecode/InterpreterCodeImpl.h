#ifndef _MATHVM_INTERPRETER_CODE_IMPL
#define _MATHVM_INTERPRETER_CODE_IMPL

#include <string>
#include <stack>
#include <cstring>
#include "stdint.h"
#include "mathvm.h"
#include <tr1/memory>

namespace mathvm {

using std::string;
using std::stack;
using std::tr1::shared_ptr;


class InterpreterCodeImpl: public Code {
	stack< shared_ptr<Var> > programStack;
	stack<Bytecode*> bytecodes;
	stack<size_t> bcis;
	void getBytecode();

	double popDouble();
	int64_t popInt();
	const char *popString();
	shared_ptr<Var> pop();

	void push(shared_ptr<Var> var);
	void runBytecode(Bytecode *bytecode);
	
	shared_ptr<Var> getDoubleVar(double val);
	shared_ptr<Var> getIntVar(int64_t val);
	shared_ptr<Var> getStringVar(const char* val);
	
	shared_ptr<Var> mem[1 << 16];
	
	

public:

	Status* execute(vector<Var*>& vars);
};



}

#endif
