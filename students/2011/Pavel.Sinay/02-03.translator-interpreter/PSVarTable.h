/*
 * PSVarTable.h
 *
 *  Created on: 18.01.2012
 *      Author: Pavel Sinay
 */

#ifndef PSVARTABLE_H_
#define PSVARTABLE_H_

#include <vector>
#include <map>
#include "StackVar.h"

class PSVarTable {
public:
	PSVarTable();
	virtual ~PSVarTable();

	void openPage();
	void closePage();

	void addVar(std::string const& name);
	uint16_t getVarAddr(std::string name);

	void dump() const;

private:
	std::vector<std::map<std::string, uint16_t> > m_pages;
	uint16_t m_var_addr;
};

#endif /* PSVARTABLE_H_ */
