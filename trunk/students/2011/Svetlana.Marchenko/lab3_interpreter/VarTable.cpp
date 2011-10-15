#include "VarTable.h"
#include <stdio.h>


uint8_t VarTable::getIdByName(const string& varName) {
	return _vars[varName];
}

uint8_t VarTable::addVar(const string& varName) {
	_vars[varName] = _counter++; 
  return _counter-1;
}
