/*
 * ExecStack.cpp
 *
 *  Created on: 18.01.2012
 *      Author: Pavel Sinay
 */

#include "ExecStack.h"
#include "MVException.h"
#include <iostream>

using namespace mathvm;

ExecStack::ExecStack() {
	// TODO Auto-generated constructor stub

}

ExecStack::~ExecStack() {
	// TODO Auto-generated destructor stub
}

void ExecStack::pushInt(uint64_t i) {
	Var var(VT_INT, "");
	var.setIntValue(i);
	m_stack.push_back(var);
}

void ExecStack::pushDouble(double d) {
	Var var(VT_DOUBLE, "");
	var.setDoubleValue(d);
	m_stack.push_back(var);
}

void ExecStack::pushString(std::string s) {
	Var var(VT_STRING, "");
	var.setStringValue(s.c_str());
	m_stack.push_back(var);
}

long int ExecStack::popInt() {
	Var var = pop();
	return var.getIntValue();
}

double ExecStack::popDouble() {
	Var var = pop();
	return var.getDoubleValue();
}

std::string ExecStack::popString() {
	Var var = pop();
	return var.getStringValue();
}

Var ExecStack::pop() {
	if (m_stack.size() == 0) {
		throw(MVException("Stack is empty"));
	}
	Var var = m_stack.back();
	m_stack.pop_back();
	return var;
}
