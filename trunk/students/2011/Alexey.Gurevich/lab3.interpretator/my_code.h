#pragma once

#include "ast.h"
#include <string.h>
#include <vector>
#include <map>

namespace mathvm {

class MyCode : public Code {

	typedef Var*		var_ptr_t;
	typedef var_ptr_t*	func_vars_t;

	union StackValue {
		double  	doubleValue;
		int64_t 	intValue;
		const char* stringValue;
	};

	void pushDouble (double);
	void pushInt    (int64_t);
	void pushString (const char*);
	void pushValue  (StackValue);

	double   	 popDouble();
	int64_t  	 popInt();
	const char*  popString();
	StackValue   popValue();

	// for functions handling
	Status* executeBytecode(Bytecode* bytecode);

public:
	MyCode() {}
	~MyCode() {}
	Status* execute(vector<Var*>& vars);

	map<uint16_t, uint16_t>* getFuncVarsCount() {
		return &funcVarsCount_;
	}

private:
	map<uint16_t, uint16_t> funcVarsCount_;
	vector<StackValue>  stack_;
	func_vars_t* varsStorage_;
};

}
