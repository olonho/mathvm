/*
 * context.h
 *
 *  Created on: 19.11.2012
 *      Author: Evgeniy Krasko
 */

#ifndef CONTEXT_H_
#define CONTEXT_H_

union stacktype {
	int64_t i64;
	double d;
	int16_t i16;
	stacktype() {

	}
	stacktype(double d) :
			d(d) {
	}
	stacktype(int64_t i) :
			i64(i) {
	}
	stacktype(int16_t i) :
			i16(i) {
	}
	stacktype(const stacktype & st) {
		i64 = st.i64;
	}
	operator int64_t() {
		return i64;
	}
	operator double() {
		return d;
	}
	operator int16_t() {
		return i16;
	}
};

class Context {
	vector<vector<stacktype> > variables;
	vector<int> varCounts;
	vector<BytecodeFunction *> funcs;
public:
	Context() {
	}

	void setFunction(BytecodeFunction * f) {
		int size = variables.size();
		if (f->id() >= size) {
			size = f->id() + 1;
			variables.resize(size);
			varCounts.resize(size);
			funcs.resize(size);
		}
		funcs[f->id()] = f;
		varCounts[f->id()] = f->localsNumber();
	}

	stacktype get(int16_t fun, int16_t var) {
		return variables[fun][variables[fun].size() - var];
	}

	void set(int16_t fun, int16_t var, stacktype value) {
		variables[fun][variables[fun].size() - var] = value;
	}

	BytecodeFunction * enter(int16_t funIndex) {
		int oldSize = variables[funIndex].size();
		variables[funIndex].resize(oldSize + varCounts[funIndex]);
		return funcs[funIndex];
	}

	void leave(int16_t funIndex) {
		int oldSize = variables[funIndex].size();
		variables[funIndex].resize(oldSize - varCounts[funIndex]);
	}
};

#endif /* CONTEXT_H_ */
