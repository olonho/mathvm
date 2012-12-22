#ifndef _MATHVM_INTERPRETER_CODE_IMPL
#define _MATHVM_INTERPRETER_CODE_IMPL

#include <string>
#include <stack>
#include <cstring>
#include "stdint.h"
#include "mathvm.h"


namespace mathvm {

using std::string;
using std::stack;


class InterpreterCodeImpl: public Code {
	stack<Var*> programStack;
	stack<Bytecode*> bytecodes;
	stack<size_t> bcis;
	void getBytecode();

	double popDouble();
	int64_t popInt();
	const char *popString();
	Var* pop();

	void push(Var *val);
	void runBytecode(Bytecode *bytecode);
	
	Var* getDoubleVar(double val);
	Var* getIntVar(int64_t val);
	Var* getStringVar(const char* val);
	
	Var* mem[1 << 16];
	
	

public:

	Status* execute(vector<Var*>& vars);
};



}

#endif
