/*
 * BytecodeBuilder.cpp
 *
 *  Created on: 24.10.2012
 *      Author: temp1ar
 */

#include "BytecodeBuilder.h"
#include "BytecodeImpl.h"
#include "BytecodeVisitor.h"
#include "BytecodeImpl.h"
#include "parser.h"
#include <memory>

namespace mathvm {
Status* BytecodeTranslatorImpl::translate(const string& program, Code** code) {
	return 0;
}

Status* BytecodeBuilder::translate(const string& program, Code** code) {
	Status* parseStatus;

	Parser parser;
	parseStatus = parser.parseProgram(program);

	if(parseStatus != 0 && parseStatus->isError()) {
		cerr << "Error in parsing source." << endl;
		return parseStatus;
	}

	*code = new BytecodeImpl();

	BytecodeVisitor visitor(parser.top(), *code);
	visitor.visit();

	return 0;
}

}


