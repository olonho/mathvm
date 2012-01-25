#pragma once
#include "ast.h"
#include "config.h"
#include "my_code.h"
#include "AsmJit/Compiler.h"
#include "AsmJit/AsmJit.h"
#include <vector>
#include <map>
#include <set>

namespace mathvm {

class MyJitTranslator : public MachCodeTranslatorImpl {

	Status* translateMachCode(const string& program, MyMachCodeImpl* *code);
	Status* generateFunction(Code* code, BytecodeFunction* bytecode, void* *function);
	Status* generate(Code* code, MyMachCodeImpl* *mach_code);

public:
	typedef sysint_t	var_t;
	typedef var_t*		func_vars_t;
	typedef void* 		func_t;

	MyJitTranslator() {}
	~MyJitTranslator() {

		uint16_t funcNumber = funcVarsCount_.size();
		for (uint16_t i = 1; i != funcNumber; ++i) {
			varsStorage_[i] = (varsStorage_[i]) + funcVarsCount_[i];
			delete [] varsStorage_[i];
		}
		delete [] varsStorage_;

		delete [] funcStorage_;
	}

	Status* translate(const string& program, Code* *code);

private:
	map<uint16_t, uint16_t>	funcVarsCount_;
	map<uint16_t, void*> 	functions_;
	func_vars_t* 			varsStorage_;
	func_t* 				funcStorage_;
};

}
