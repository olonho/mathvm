/*
 * BytecodeImpl.h
 *
 *  Created on: 24.10.2012
 *      Author: temp1ar
 */

#ifndef BYTECODEIMPL_H_
#define BYTECODEIMPL_H_

#include "mathvm.h"

namespace mathvm {

union value {
    int64_t i;
    double d;
    uint16_t sId;
};

class BytecodeImpl: public Code {
    map<pair<uint16_t, uint16_t>, uint64_t> storedIntsCustom_;
    map<pair<uint16_t, uint16_t>, double> storedDoublesCustom_;
    map<pair<uint16_t, uint16_t>, uint16_t> storedStringsCustom_;
    vector<value> stack_;

public:
	BytecodeImpl();
	virtual ~BytecodeImpl();

	virtual Status* execute(vector<Var*>& vars);

	void executeFunction(BytecodeFunction* f);
};

template<typename T>
uint64_t compare(T v1, T v2) {
	if (v1 > v2)
		return 1;
	if (v1 == v2)
		return 0;
	if (v1 < v2)
		return -1;
	return 0;
}

} /* namespace mathvm */
#endif /* BYTECODEIMPL_H_ */
