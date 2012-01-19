/*
 * Code.cpp
 *
 *  Created on: 17.01.2012
 *      Author: Pavel Sinay
 */

#include "../../../../include/mathvm.h"
#include "Code.h"
#include "MVException.h"
#include <string.h>
using namespace mathvm;

PSCode::PSCode() {
	// TODO Auto-generated constructor stub
}

PSCode::~PSCode() {
	// TODO Auto-generated destructor stub
}

Status* PSCode::execute(std::vector<Var*>& vars) {
	m_var_table.openPage();
	uint16_t position = 0;
	while (position != m_bytecode.length()) {
		Instruction inst = m_bytecode.getInsn(position++);
		switch (inst) {
		case BC_IPRINT: {
			uint64_t value = m_stack.popInt();
			std::cout << (long int) value;
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
			uint16_t id = m_bytecode.getInt16(position);
			m_stack.pushString(constantById(id));
			position += sizeof(uint16_t);
			break;
		}

		case BC_STOREIVAR: {
			uint16_t addr = m_bytecode.getInt16(position);
			position += sizeof(uint16_t);
			Var var(VT_INT, "");
			var.setIntValue(m_stack.popInt());
			m_var_table.setVar(var, addr);
			break;
		}

		case BC_STOREDVAR: {
			uint16_t addr = m_bytecode.getInt16(position);
			position += sizeof(uint16_t);
			Var var(VT_DOUBLE, "");
			var.setDoubleValue(m_stack.popDouble());
			m_var_table.setVar(var, addr);
			break;
		}

		case BC_STORESVAR: {
			uint16_t addr = m_bytecode.getInt16(position);
			position += sizeof(uint16_t);
			Var var(VT_STRING, "");
			std::string tmp_str = m_stack.popString();
			char *tmp_string_p = new char[tmp_str.size() + 1];
			memset(tmp_string_p, 0, tmp_str.size() + 1);
			memcpy(tmp_string_p, tmp_str.c_str(), tmp_str.size()); //fuck off...
			var.setStringValue(tmp_string_p);
			m_var_table.setVar(var, addr);
			break;
		}

		case BC_LOADIVAR: {
			uint16_t addr = m_bytecode.getInt16(position);
			position += sizeof(uint16_t);
			m_stack.pushInt(m_var_table.getVar(addr).getIntValue());
			break;
		}

		case BC_LOADDVAR: {
			uint16_t addr = m_bytecode.getInt16(position);
			position += sizeof(uint16_t);
			m_stack.pushDouble(m_var_table.getVar(addr).getDoubleValue());
			break;
		}

		case BC_LOADSVAR: {
			uint16_t addr = m_bytecode.getInt16(position);
			position += sizeof(uint16_t);
			m_stack.pushString(m_var_table.getVar(addr).getStringValue());
			break;
		}

		case BC_STORECTXIVAR: {
			uint16_t addr = m_bytecode.getInt16(position);
			position += sizeof(uint16_t);
			position += sizeof(uint16_t);
			m_var_table.allocVar(Var(VT_INT, ""), addr);
			break;
		}

		case BC_STORECTXDVAR: {
			uint16_t addr = m_bytecode.getInt16(position);
			position += sizeof(uint16_t);
			position += sizeof(uint16_t);
			m_var_table.allocVar(Var(VT_DOUBLE, ""), addr);
			break;
		}

		case BC_STORECTXSVAR: {
			uint16_t addr = m_bytecode.getInt16(position);
			position += sizeof(uint16_t);
			position += sizeof(uint16_t);
			m_var_table.allocVar(Var(VT_STRING, ""), addr);
			break;
		}

		case BC_INEG: {
			m_stack.pushInt(-m_stack.popInt());
			break;
		}

		case BC_DNEG: {
			m_stack.pushDouble(-m_stack.popDouble());
			break;
		}

		case BC_ILOAD0: {
			m_stack.pushInt(0);
			break;
		}

		case BC_JA: {
			position += (short) m_bytecode.getInt16(position);
			break;
		}

		case BC_IFICMPNE: {
			short jump_addr = (short) m_bytecode.getInt16(position)
					- sizeof(uint16_t);
		//	std::cout << ">>> JA=" << jump_addr << std::endl;
			position += sizeof(uint16_t);
			if (m_stack.popInt() != m_stack.popInt()) {
				position += jump_addr;
			}
			break;
		}

		case BC_IADD: {
			m_stack.pushInt(m_stack.popInt() + m_stack.popInt());
			break;
		}

		case BC_DADD: {
			m_stack.pushDouble(m_stack.popDouble() + m_stack.popDouble());
			break;
		}

		case BC_IMUL: {
			m_stack.pushInt(m_stack.popInt() * m_stack.popInt());
			break;
		}

		case BC_DMUL: {
			m_stack.pushDouble(m_stack.popDouble() * m_stack.popDouble());
			break;
		}

		case BC_ISUB: {
			m_stack.pushInt(m_stack.popInt() - m_stack.popInt());
			break;
		}

		case BC_DSUB: {
			m_stack.pushDouble(m_stack.popDouble() - m_stack.popDouble());
			break;
		}

		case BC_IDIV: {
			m_stack.pushInt(m_stack.popInt() / m_stack.popInt());
			break;
		}

		case BC_DDIV: {
			m_stack.pushDouble(m_stack.popDouble() / m_stack.popDouble());
			break;
		}

		case BC_IMOD: {
			m_stack.pushInt(m_stack.popInt() % m_stack.popInt());
			break;
		}

		case BC_STOP: {
			//break;
		}
		default:
			//break;
			throw MVException(
					"BYTE code is not implemented at position = " + intToStr(
							position), position);
		}
		//break;
	}
	//std::cerr << std::endl;
	//m_var_table.dump();
	m_var_table.closePage();
	return new Status;
}

void PSCode::setByteCode(mathvm::Bytecode const &bytecode) {
	m_bytecode = bytecode;
}
