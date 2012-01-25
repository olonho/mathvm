#pragma once

#include "ast.h"
#include <string.h>
#include <vector>
#include <map>

namespace mathvm {

class MyCode : public Code {

	typedef Var*		var_ptr_t;
	typedef var_ptr_t*	func_vars_t;

public:
	MyCode() {}
	~MyCode() {}
	Status* execute(vector<Var*>& vars);

	map<uint16_t, uint16_t>* getFuncVarsCount() {
		return &funcVarsCount_;
	}

private:
	map<uint16_t, uint16_t> funcVarsCount_;
	func_vars_t* varsStorage_;
};

class MyMachCodeImpl : public Code {
	void* _code;

public:
	MyMachCodeImpl();
	~MyMachCodeImpl();

	Status* execute(vector<Var*>& vars);
	void setCode(void* code) { _code = code; }
};

}
