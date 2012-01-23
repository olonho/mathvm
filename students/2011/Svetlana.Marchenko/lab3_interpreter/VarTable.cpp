#include "VarTable.h"
#include <stdio.h>


uint16_t VarTable::tryFindIdByName(const string& varName) {
	uint16_t i = 0;
	while ( i < _vars.size()) {
		if (!_vars[i].compare(varName)) return i+_startIndex;
		++i;
	}
	return 0;
}

uint16_t VarTable::addVar(const string& varName) {
	_vars.push_back(varName);
	return _vars.size()+_startIndex;
}

uint16_t VarTable::varsCount() {
	return _vars.size();
}

