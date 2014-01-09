/*
 * BCTypes.h
 *
 *  Created on: Jan 3, 2014
 *      Author: sam
 */

#ifndef BCTYPES_H_
#define BCTYPES_H_

#include <set>

namespace mathvm {
typedef union {
	int64_t vInt;
	double vDouble;
	const char* vStr;
} Value;
typedef union {
	int16_t* pJmp;
	uint16_t* pVar;
	uint8_t* pInsn;
	int64_t* pInt;
	double* pDouble;
} CodePtr;
typedef struct {
	CodePtr code_ptr;
	Value* vars_ptr;
	Value* stack_ptr;
} StackEntry;

typedef uint16_t VarInt;
} /* namespace mathvm */

#endif /* BCTYPES_H_ */
