/*
 * Code.h
 *
 *  Created on: 17.01.2012
 *      Author: Pavel Sinay
 */

#ifndef CODE_H_
#define CODE_H_

#include "mathvm.h"
#include "ExecStack.h"
#include "PSVarTable.h"

class PSCode: public mathvm::Code {
public:
	PSCode();
	virtual ~PSCode();
	virtual mathvm::Status* execute(std::vector<mathvm::Var*>& vars);
	void setByteCode(mathvm::Bytecode const &bytecode);

private:
	mathvm::Bytecode m_bytecode;
	ExecStack m_stack;
	VarTableExecute m_var_table;
};

#endif /* CODE_H_ */
