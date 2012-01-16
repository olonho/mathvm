#pragma once

#include <map>
#include <string>
#include <stdio.h>

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
