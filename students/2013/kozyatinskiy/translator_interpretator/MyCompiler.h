#pragma once

#include "CompilerVisitor.h"

#include <vector>
using std::vector;

class MyCompiler
{
public:
	MyCompiler(void);
	~MyCompiler(void);
	
	void* compile(const Bytecode_& bc, int16_t id, const vector<string>& literals); 
private:
	vector<void*> cache_;
	vector<bool>  cantCompile_;

	void* cantCompile(Instruction inst, int16_t id);

	static void iprint(int64_t val);
	static void sprint(const char* val);
	static void dprint(double val);
};

