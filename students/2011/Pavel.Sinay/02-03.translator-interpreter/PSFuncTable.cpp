/*
 * PSFuncTable.cpp
 *
 *  Created on: 19.01.2012
 *      Author: Pavel Sinay
 */

#include "PSFuncTable.h"
#include "MVException.h"

PSFuncTable::PSFuncTable() {
	// TODO Auto-generated constructor stub

}

PSFuncTable::~PSFuncTable() {
	// TODO Auto-generated destructor stub
}

void PSFuncTable::addFunc(FuncInfo func_info){
	std::vector<FuncInfo>::const_reverse_iterator it = m_func_table.rbegin();
	for(; it != m_func_table.rend(); ++it){
		if (func_info.name == it->name){
			throw MVException("Function '" + func_info.name + "' already exists");
		}
	}
	m_func_table.push_back(func_info);
}

FuncInfo PSFuncTable::getFuncByName(std::string const& name) const{
	std::vector<FuncInfo>::const_reverse_iterator it = m_func_table.rbegin();
	for(; it != m_func_table.rend(); ++it){
		if (name == it->name){
			return *it;
		}
	}
	throw MVException("Function '" + name + "' is not found");
}

void PSFuncTable::dump() const{
	std::cerr << "--------------- Function table dump ---------------" << std::endl;
	std::vector<FuncInfo>::const_iterator it = m_func_table.begin();
	for(; it != m_func_table.end(); ++it){
		std::cerr << it->name << " " << it->addr << std::endl;
	}
}
