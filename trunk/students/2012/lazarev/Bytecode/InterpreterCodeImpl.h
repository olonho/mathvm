#ifndef _MATHVM_INTERPRETER_CODE_IMPL
#define _MATHVM_INTERPRETER_CODE_IMPL

#include <string>
#include <stack>


using namespace mathvm;

using std::string;
using std::stack;


class Val {
	union {
		double doubleVal;
		int64_t intVal;
		string stringVal;
	}

public:
	Val(double doubleVal): doubleVal(doubleVal) {}
	Val(int intVal): intVal(intVal) {}
	Val(string& stringVal): stringVal(stringVal) {}	

	double getDouble() {
		return doubleVal;
	}

	int getInt() {
		return intVal;
	}

	string getString() {
		return stringVal;
	}
};


class InterpreterCodeImpl: public Code {
	stack<Val> programStack;
	void getBytecode();

	double popDouble();
	int popInt();
	string popString();

	void push(Val& val);

public:

	Status* execute(vector<Var*>& vars);
};

#endif