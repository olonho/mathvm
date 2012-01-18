/*
 * Code.cpp
 *
 *  Created on: 17.01.2012
 *      Author: Pavel Sinay
 */

#include "../../../../include/mathvm.h"
#include "Code.h"
#include "MVException.h"
using namespace mathvm;

PSCode::PSCode() {
	// TODO Auto-generated constructor stub
}

PSCode::~PSCode() {
	// TODO Auto-generated destructor stub
}

Status* PSCode::execute(std::vector<Var*>& vars) {
	uint32_t position = 0;
	while (position != m_bytecode.length()) {
		Instruction inst = m_bytecode.getInsn(position++);
		switch (inst) {
		case BC_IPRINT: {
			uint64_t value = m_stack.popInt();
			std::cout << value;
			break;
		}
		case BC_DPRINT: {
			double value = m_stack.popDouble();
			std::cout << value;
			break;
		}
		case BC_SPRINT: {
			std::string str = m_stack.popString();
			std::cout << str;
			break;
		}
		case BC_ILOAD: {
			m_stack.pushInt(m_bytecode.getInt64(position));
			position += sizeof(uint64_t);
			break;
		}
		case BC_DLOAD: {
			m_stack.pushDouble(m_bytecode.getDouble(position));
			position += sizeof(double);
			break;
		}
		case BC_SLOAD: {
			uint16_t size = m_bytecode.getInt16(position);
			position += sizeof(uint16_t);
			std::string str;
			str.resize(size);
			for (uint32_t i = position; i < position + size; ++i) {
				str[i - position] = m_bytecode.getByte(i);
			}
			position += size;
			m_stack.pushString(str);
			break;
		}

		default:
			//break;
			throw MVException("code is not implemented");
		}
		//break;
	}
	return new Status;
}

void PSCode::setByteCode(mathvm::Bytecode const &bytecode) {
	m_bytecode = bytecode;
}
