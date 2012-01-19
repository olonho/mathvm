/*
 * PSFuncTable.h
 *
 *  Created on: 19.01.2012
 *      Author: Pavel Sinay
 */

#ifndef PSFUNCTABLE_H_
#define PSFUNCTABLE_H_

#include <string>
#include <stdint.h>
#include "mathvm.h"

struct FuncInfo {
	std::string name;
	mathvm::VarType result_type;
	uint16_t addr;

	FuncInfo() :
		result_type(mathvm::VT_INVALID), addr(0) {
	}

	FuncInfo(std::string name, mathvm::VarType result_type, uint16_t addr) :
		name(name), result_type(result_type), addr(addr) {
	}
};

class PSFuncTable {
public:
	PSFuncTable();
	virtual ~PSFuncTable();

	void addFunc(FuncInfo func_info);
	FuncInfo getFuncByName(std::string const& name) const;
	void dump() const;

private:
	std::vector<FuncInfo> m_func_table;
};

#endif /* PSFUNCTABLE_H_ */
