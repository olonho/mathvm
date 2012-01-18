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
#include "StackVar.h"

class ExecStack {
public:
	ExecStack();
	virtual ~ExecStack();

	void pushInt(uint64_t i);
	void pushDouble(double d);
	void pushString(std::string s);

	uint64_t popInt();
	double popDouble();
	std::string popString();

	uint64_t getInt();
	double getDouble();
	std::string getString();

private:
	StackVar *m_head;

	StackVar *pop();
	void push(StackVar *var);
};

#endif /* EXECSTACK_H_ */
