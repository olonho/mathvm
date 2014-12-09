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
	typedef map <int16_t, Var> VariableMap_idx;

	//with unique id to all program
	typedef map <string, int16_t> FunctionMapM;

	VariableMap variableMap;
	FunctionMapM functionMap;
	VariableMap_idx varMap_idx;

	Context * parent;
	string name;
	int16_t idx;


	Context(int16_t idxM, VariableMap variableMapM, FunctionMapM functionMapM, Context * parentM):
		idx(idxM), variableMap(variableMapM), functionMap(functionMapM), parent(parentM)
	{}
	Context(int16_t idxM, Context * parentM): idx(idxM), parent(parentM)
	{}

	Context(int16_t idxM, VariableMap_idx variableMapM, FunctionMapM functionMapM, Context * parentM):
		idx(idxM), varMap_idx(variableMapM), functionMap(functionMapM), parent(parentM)
	{}
};

#endif // CONTEXT_H
