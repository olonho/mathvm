#ifndef CONTEXT_H
#define CONTEXT_H
#include <iostream>
#include "mathvm.h"
using std::pair;
using std::map;
using namespace mathvm;

struct Context
{
	typedef pair<int16_t, Var> Variable;
	typedef map <string, Variable> VariableMap;

	//with unique id to all program
	typedef map <string, int16_t> FunctionMapM;

	VariableMap variableMap;
	FunctionMapM functionMap;

	Context * parent;
	string name;
	int16_t idx;


	Context(int16_t idxM, VariableMap variableMapM, FunctionMapM functionMapM, Context * parentM):
		idx(idxM), variableMap(variableMapM), functionMap(functionMapM), parent(parentM)
	{}
	Context(int16_t idxM, Context * parentM): idx(idxM), parent(parentM)
	{}
};

#endif // CONTEXT_H
