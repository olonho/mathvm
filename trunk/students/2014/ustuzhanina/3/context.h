#ifndef CONTEXT_H
#define CONTEXT_H
#include <iostream>
#include "mathvm.h"
using std::pair;
using std::map;
using namespace mathvm;

struct Context
{
    typedef pair<uint16_t, Var> Variable;
	typedef map <string, Variable> VariableMap;
    typedef map <uint16_t, Var> VariableMap_idx;

	//with unique id to all program
    typedef map <string, uint16_t> FunctionMapM;

	VariableMap variableMap;
	FunctionMapM functionMap;
	VariableMap_idx varMap_idx;

	Context * parent;
	string name;
    uint16_t idx;


    Context(uint16_t idxM, VariableMap variableMapM, FunctionMapM functionMapM, Context * parentM):
		idx(idxM), variableMap(variableMapM), functionMap(functionMapM), parent(parentM)
	{}
    Context(uint16_t idxM, Context * parentM): idx(idxM), parent(parentM)
	{}

    Context(uint16_t idxM, VariableMap_idx variableMapM, FunctionMapM functionMapM, Context * parentM):
		idx(idxM), varMap_idx(variableMapM), functionMap(functionMapM), parent(parentM)
	{}
};

#endif // CONTEXT_H
