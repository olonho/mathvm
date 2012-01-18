/*
 * StackVar.h
 *
 *  Created on: 18.01.2012
 *      Author: Pavel Sinay
 */

#ifndef STACKVAR_H_
#define STACKVAR_H_

#include <stdint.h>
#include <string>

struct StackVar {
	StackVar(int id) :
		pred(NULL), id(id) {
	}
	;
	~StackVar() {
	}
	;

	StackVar *pred;
	int id;
};

struct StackVarInt: StackVar {
	StackVarInt(uint64_t i) :
		StackVar(1), i(i) {
	}
	;
	~StackVarInt() {
	}
	;
	static const int etalon_id = 1;
	uint64_t i;
};

struct StackVarDouble: StackVar {
	StackVarDouble(double d) :
		StackVar(2), d(d) {
	}
	;
	~StackVarDouble() {
	}
	;
	double d;
	static const int etalon_id = 2;
};

struct StackVarString: StackVar {
	StackVarString(std::string s) :
		StackVar(3), s(s) {
	}
	;
	~StackVarString() {
	}
	;
	std::string s;
	static const int etalon_id = 3;
};

#endif /* STACKVAR_H_ */
