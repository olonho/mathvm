#pragma once

#include <map>
#include <string>
#include <stdio.h>

using std::map;
using std::string;

typedef unsigned char uint8_t;


class VarTable {
	map<string, uint8_t> _vars;
	uint8_t _counter;
	public:
		VarTable(): 
			_counter(0) 
		{}
		uint8_t getIdByName(const string& varName);
		uint8_t addVar(const string& varName);
	
};
