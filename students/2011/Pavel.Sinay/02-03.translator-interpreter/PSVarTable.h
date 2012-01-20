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
#include "mathvm.h"

struct VarWithAddr: mathvm::Var {
	uint16_t addr;

	VarWithAddr(mathvm::Var const& var) :
		Var(var), addr(0) {
	}
	;
};

class PSVarTable {
public:
	PSVarTable();
	virtual ~PSVarTable();

	void openPage();
	void closePage();

	void dump() const;

protected:
	std::vector<std::vector<VarWithAddr> > m_pages_;
};

class PSVarTableTranslate: public PSVarTable {
public:

	PSVarTableTranslate() :
	m_var_addr(0) {}
	void addVar(mathvm::Var var);
	uint16_t getVarAddr(std::string name);
private:
	uint16_t m_var_addr;
};

class VarTableExecute : public PSVarTable {
public:
	void allocVar(mathvm::Var var, uint16_t addr);
	void setVar(mathvm::Var var, uint16_t addr);
	VarWithAddr getVar(uint16_t addr);
private:
	std::vector<mathvm::Var*> m_topmost_vars;
};

#endif /* PSVARTABLE_H_ */
