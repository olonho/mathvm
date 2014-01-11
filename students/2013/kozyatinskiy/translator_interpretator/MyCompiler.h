#pragma once

#include "CompilerVisitor.h"

#include <vector>
using std::vector;

class MyCompiler
{
public:
	MyCompiler(void);
	~MyCompiler(void);
	
	void* compile(const Bytecode_& bc, int16_t id); 
private:
	vector<void*> cache_;
	vector<bool>  cantCompile_;

	void* cantCompile(Instruction inst, int16_t id);
};

