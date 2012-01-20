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

void PSFuncTable::openPage() {
	m_func_table.resize(m_func_table.size() + 1);
}

void PSFuncTable::closePage() {
	m_func_table.pop_back();
}

void PSFuncTable::addFunc(FuncInfo func_info) {
		std::vector<FuncInfo>::const_reverse_iterator func_it =
				m_func_table.back().rbegin();
		for (; func_it != m_func_table.back().rend(); ++func_it) {
			if (func_info.name == func_it->name) {
				throw MVException(
						"Function '" + func_info.name + "' already exists");
			}
		}
	(m_func_table.end() - 2)->push_back(func_info);
}

FuncInfo PSFuncTable::getFuncByName(std::string const& name) const {
	std::vector<std::vector<FuncInfo> >::const_reverse_iterator page_it =
			m_func_table.rbegin();
	for (; page_it != m_func_table.rend(); ++page_it) {

		std::vector<FuncInfo>::const_reverse_iterator func_it =
				page_it->rbegin();
		for (; func_it != page_it->rend(); ++func_it) {
			if (name == func_it->name) {
				return *func_it;
			}
		}
	}
	throw MVException("Function '" + name + "' is not found");
}

void PSFuncTable::dump() const {
	std::cerr << "--------------- Function table dump ---------------"
			<< std::endl;

	std::vector<std::vector<FuncInfo> >::const_reverse_iterator page_it =
			m_func_table.rbegin();
	int page_counter = 0;
	for (; page_it != m_func_table.rend(); ++page_it) {
		std::cerr << "Page " << page_counter++ << std::endl;
		std::vector<FuncInfo>::const_reverse_iterator func_it =
				page_it->rbegin();
		for (; func_it != page_it->rend(); ++func_it) {
			std::cerr << "  " << func_it->name << " " << func_it->addr << std::endl;
		}
	}
	std::cerr << "--------------- Function table dump end ---------------" << std::endl;
}
