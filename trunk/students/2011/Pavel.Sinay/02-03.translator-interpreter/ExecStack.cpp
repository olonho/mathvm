/*
 * ExecStack.cpp
 *
 *  Created on: 18.01.2012
 *      Author: Pavel Sinay
 */

#include "ExecStack.h"
#include "MVException.h"
#include <iostream>

ExecStack::ExecStack() {
	// TODO Auto-generated constructor stub

}

ExecStack::~ExecStack() {
	// TODO Auto-generated destructor stub
}

void ExecStack::pushInt(uint64_t i) {
	StackVarInt *si = new StackVarInt(i);
	push(si);
}

void ExecStack::pushDouble(double d) {
	StackVarDouble *sd = new StackVarDouble(d);
	push(sd);
}

void ExecStack::pushString(std::string s) {
	StackVarString *ss = new StackVarString(s);
	push(ss);
}

uint64_t ExecStack::popInt() {
	StackVarInt *si = static_cast<StackVarInt*>(pop());
	int i = si->i;
	delete si;
	if (si->id != StackVarInt::etalon_id){
		throw(MVException("Stack variable is not int"));
	}
	return i;
}

double ExecStack::popDouble() {
	StackVarDouble *sd = static_cast<StackVarDouble*>(pop());
	double d = sd->d;
	delete sd;
	if (sd->id != StackVarDouble::etalon_id){
		throw(MVException("Stack variable is not double"));
	}
	return d;
}

std::string ExecStack::popString() {
	StackVarString *ss = static_cast<StackVarString*>(pop());
	std::string s = ss->s;
	delete ss;
	if (ss->id != StackVarString::etalon_id){
		throw(MVException("Stack variable is not string"));
	}
	return s;
}

StackVar *ExecStack::pop() {
	if (m_head == NULL){
		throw MVException("Bottom of stack");
	}
	StackVar *var = m_head;
	m_head = m_head->pred;
	if (m_head == NULL){
		//throw MVException("Bottom of stack");
	}
	return var;
}

void ExecStack::push(StackVar *var) {
		var->pred = m_head;
		m_head = var;
}
