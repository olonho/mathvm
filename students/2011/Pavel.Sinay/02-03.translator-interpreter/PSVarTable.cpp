/*
 * PSVarTable.cpp
 *
 *  Created on: 18.01.2012
 *      Author: Pavel Sinay
 */

#include <iostream>
#include "PSVarTable.h"
#include "MVException.h"

using namespace mathvm;

PSVarTable::PSVarTable() {
	// TODO Auto-generated constructor stub

}

PSVarTable::~PSVarTable() {
	// TODO Auto-generated destructor stub
}

void PSVarTable::openPage() {
	m_pages_.resize(m_pages_.size() + 1);
}

void PSVarTable::closePage() {
	m_pages_.pop_back();
}

void PSVarTable::dump() const {
	std::cerr << "--------------Variable table dump--------------" << std::endl;
	std::vector<std::vector<VarWithAddr> >::const_iterator it_page =
			m_pages_.begin();
	int page = 0;
	for (; it_page != m_pages_.end(); ++it_page) {
		std::cerr << "Page " << page++ << std::endl;
		std::vector<VarWithAddr>::const_iterator it = it_page->begin();
		for (; it != it_page->end(); ++it) {
			std::cerr << "  " << it->name() << " " << it->addr << std::endl;
		}
		std::cerr << std::endl;
	}
}

void PSVarTableTranslate::addVar(mathvm::Var var) {
	std::vector<VarWithAddr>::iterator it = m_pages_.back().begin();
	for (; it != m_pages_.back().end(); ++it) {
		if (it->name() == var.name()) {
			throw MVException("Variable '" + var.name() + "' already declared");
		}
	}

	VarWithAddr var_a = var;
	var_a.addr = m_var_addr++;
	m_pages_.back().push_back(var_a);
}

uint16_t PSVarTableTranslate::getVarAddr(std::string name) {
	//dump();
	std::vector<std::vector<VarWithAddr> >::const_reverse_iterator it_page =
			m_pages_.rbegin();
	for (; it_page != m_pages_.rend(); ++it_page) {
		std::vector<VarWithAddr>::const_iterator it = it_page->begin();
		for (; it != it_page->end(); ++it) {
			if (it->name() == name) {
				return it->addr;
			}
		}
	}

	throw MVException("Variable '" + name + "' is not declared");
}

void VarTableExecute::allocVar(mathvm::Var var, uint16_t addr){
	VarWithAddr var_a = var;
	var_a.addr = addr;
	m_pages_.back().push_back(var_a);
}

void VarTableExecute::setVar(Var var, uint16_t addr) {

	std::vector<std::vector<VarWithAddr> >::reverse_iterator it_page =
			m_pages_.rbegin();
	for (; it_page != m_pages_.rend(); ++it_page) {
		std::vector<VarWithAddr>::iterator it = it_page->begin();
		for (; it != it_page->end(); ++it) {
			if (it->addr == addr) {

				switch (it->type()) {
				case VT_INT:
					it->setIntValue(var.getIntValue());
					return;
				case VT_DOUBLE:
					it->setDoubleValue(var.getDoubleValue());
					return;
				case VT_STRING:
					it->setStringValue(var.getStringValue());
					return;
				default:
					throw MVException("Invalid variable type ,addr = " + intToStr(addr));
				}
			}
		}
	}

	throw MVException("Variable with address '" + intToStr(addr) + "' is not found");
}

VarWithAddr VarTableExecute::getVar(uint16_t addr) {
	std::vector<std::vector<VarWithAddr> >::reverse_iterator it_page =
			m_pages_.rbegin();
	for (; it_page != m_pages_.rend(); ++it_page) {
		std::vector<VarWithAddr>::iterator it = it_page->begin();
		for (; it != it_page->end(); ++it) {
			if (it->addr == addr) {
				return *it;
			}
		}
	}
	throw MVException("Variable with address '" + intToStr(addr) + "' is not found");
}

