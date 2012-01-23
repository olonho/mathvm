#pragma once

#include <map>
#include <string>
#include <vector>
#pragma once

#include <stdio.h>
#include <stdint.h>
#include <map>
#include <vector>

using std::map;
using std::vector;
using std::string;

class VarTable {
	vector<string> _vars;
	uint16_t _startIndex;

	public:
		VarTable(int startIndex): 
			_startIndex(startIndex)
		{}
		
		uint16_t tryFindIdByName(const string& varName);
		uint16_t addVar(const string& varName);
		uint16_t varsCount();
};

class FunctionTable {
	map<string, uint16_t> _funcs;
	uint16_t _counter;
	public:
		FunctionTable(): 
			_counter(0) 
		{}
		FunctionTable(uint16_t init_counter_val): 
			_counter(init_counter_val) 
		{}
		bool getIdByName(const string& funcName, uint16_t& returnId);
		bool addFunction(const string& funcName, uint16_t& returnId);
	
};
