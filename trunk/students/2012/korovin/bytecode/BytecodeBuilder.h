/*
 * BytecodeBuilder.h
 *
 *  Created on: 24.10.2012
 *      Author: temp1ar
 */

#ifndef BYTECODEBUILDER_H_
#define BYTECODEBUILDER_H_

#include "ast.h"
#include "mathvm.h"

namespace mathvm {

class BytecodeBuilder: public BytecodeTranslatorImpl {
public:
    Status* translate(const string& program, Code* *code);
};

}
#endif /* BYTECODEBUILDER_H_ */
