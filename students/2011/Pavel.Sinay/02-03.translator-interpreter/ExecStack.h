/*
 * ExecStack.h
 *
 *  Created on: 18.01.2012
 *      Author: Pavel Sinay
 */

#ifndef EXECSTACK_H_
#define EXECSTACK_H_

#include <string>
#include <stdint.h>
#include "mathvm.h"

class ExecStack {
public:
	ExecStack();
	virtual ~ExecStack();

	void pushInt(uint64_t i);
	void pushDouble(double d);
	void pushString(std::string s);

	long int popInt();
	double popDouble();
	std::string popString();

	uint64_t getInt();
	double getDouble();
	std::string getString();

private:
	std::vector<mathvm::Var> m_stack;

	mathvm::Var pop();
};

#endif /* EXECSTACK_H_ */
