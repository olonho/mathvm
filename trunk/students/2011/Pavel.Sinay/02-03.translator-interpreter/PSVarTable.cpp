/*
 * PSVarTable.cpp
 *
 *  Created on: 18.01.2012
 *      Author: Pavel Sinay
 */

#include <iostream>
#include "PSVarTable.h"
#include "MVException.h"

PSVarTable::PSVarTable() :
	m_var_addr(0) {
	// TODO Auto-generated constructor stub

}

PSVarTable::~PSVarTable() {
	// TODO Auto-generated destructor stub
}

void PSVarTable::openPage() {
	m_pages.resize(m_pages.size() + 1);
}

void PSVarTable::closePage() {
	m_pages.pop_back();
}

void PSVarTable::addVar(std::string const& name) {
	if (m_pages.back().find(name) != m_pages.back().end()){
		throw MVException("Variable '" + name + "' already declared");
	}
	m_pages.back().insert(std::make_pair(name, m_var_addr++));
}

uint16_t PSVarTable::getVarAddr(std::string name) {
	std::vector<std::map<std::string, uint16_t> >::const_reverse_iterator it = m_pages.rend();
	for(; it != m_pages.rbegin(); --it){
		if (it->find(name) != it->end()){
			return it->find(name)->second;
		}
	}

	throw MVException("Variable '" + name + "' is not declared");
}

void PSVarTable::dump() const{
	std::cout << "--------------Variable table dump--------------" << std::endl;
	std::vector<std::map<std::string, uint16_t> >::const_iterator it_pages = m_pages.begin();

	int page = 0;
	for(; it_pages != m_pages.end(); ++it_pages){
		std::cout << "Page " << page++ << std::endl;
		std::map<std::string, uint16_t>::const_iterator it_var = it_pages->begin();
		for(; it_var != it_pages->end(); ++it_var){
			std::cout << "  " << it_var->first << " " << it_var->second << std::endl;
		}
		std::cout << std::endl;
	}
}
