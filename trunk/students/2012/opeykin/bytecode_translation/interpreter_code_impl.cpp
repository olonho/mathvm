/*
 * interpreter_code_impl.cpp
 *
 *  Created on: Nov 20, 2012
 *      Author: alex
 */

#include "interpreter_code_impl.h"

namespace mathvm {

InterpreterCodeImpl::InterpreterCodeImpl() : _out(cout) {
}

InterpreterCodeImpl::~InterpreterCodeImpl() {
	// TODO Auto-generated destructor stub
}

BytecodeFunction* InterpreterCodeImpl::bytecodeFunctionById(uint16_t index) const {
	return static_cast<BytecodeFunction*>(functionById(index));
}
BytecodeFunction* InterpreterCodeImpl::bytecodeFunctionByName(const string& name) const {
	return static_cast<BytecodeFunction*>(functionByName(name));
}

uint16_t InterpreterCodeImpl::readUInt16FromBytecode() {
	uint16_t result = _bp->getUInt16(_ip);
	_ip += sizeof(uint16_t);
	return result;
}

int InterpreterCodeImpl::readIntFromBytecode() {
	int64_t result = _bp->getInt64(_ip);
	_ip += sizeof(int64_t);
	return result;
}

double InterpreterCodeImpl::readDoubleFromBytecode() {
	double result = _bp->getDouble(_ip);
	_ip += sizeof(double);
	return result;
}

int InterpreterCodeImpl::getIntFromTOS() {
	int value = this->_stack.top()._intValue;
	_stack.pop();
	return value;
}

double InterpreterCodeImpl::getDoubleFromTOS() {
	double value = _stack.top()._doubleValue;
	_stack.pop();
	return value;
}

void InterpreterCodeImpl::pushIntToTOS(int value) {
	StackUnit unit;
	unit._intValue = value;
	_stack.push(unit);
}

void InterpreterCodeImpl::pushDoubleToTOS(double value) {
	StackUnit unit;
	unit._doubleValue = value;
	_stack.push(unit);
}

void InterpreterCodeImpl::loadIntVar(uint32_t index) {
	StackUnit unit;
	Var* var = _context->getVar(index);
	unit._intValue = var->getIntValue();
	_stack.push(unit);
}

void InterpreterCodeImpl::loadDoubleVar(uint32_t index) {
	StackUnit unit;
	Var* var = _context->getVar(index);
	unit._doubleValue = var->getDoubleValue();
	_stack.push(unit);
}

void InterpreterCodeImpl::storeIntVar(uint32_t index) {
	Var* var = _context->getVar(index);
	var->setIntValue(getIntFromTOS());
}

void InterpreterCodeImpl::storeDoubleVar(uint32_t index) {
	Var* var = _context->getVar(index);
	var->setDoubleValue(getDoubleFromTOS());
}

void InterpreterCodeImpl::jump() {
	_ip += _bp->getInt16(_ip);
}

Status* InterpreterCodeImpl::execute(vector<Var*>& vars) {
	BytecodeFunction* top = bytecodeFunctionById(0);
	if (top == 0) {
		return new Status("No function to execute");
	}
	_context = new Context(0, top);
	_bp = top->bytecode();
	_ip = 0;


	while (true) {
		Instruction instruction = _bp->getInsn(_ip);
		_ip += sizeof(instruction);
		switch(instruction) {
			case BC_INVALID: return new Status("Invalid instruction"); break;
			case BC_DLOAD: pushDoubleToTOS(readDoubleFromBytecode()); break;
			case BC_ILOAD: pushIntToTOS(readIntFromBytecode()); break;
			case BC_SLOAD: break;
			case BC_DLOAD0: pushDoubleToTOS(0.0); break;
			case BC_ILOAD0: pushIntToTOS(0); break;
			case BC_SLOAD0: break;
			case BC_DLOAD1: pushDoubleToTOS(1.0); break;
			case BC_ILOAD1: pushIntToTOS(1); break;
			case BC_DLOADM1: pushDoubleToTOS(-1.0); break;
			case BC_ILOADM1: pushIntToTOS(-1); break;
			case BC_DADD: pushDoubleToTOS(getDoubleFromTOS() + getDoubleFromTOS()); break;
			case BC_IADD: pushIntToTOS(getIntFromTOS() + getIntFromTOS()); break;
			case BC_DSUB: pushDoubleToTOS(getDoubleFromTOS() - getDoubleFromTOS()); break;
			case BC_ISUB: pushIntToTOS(getIntFromTOS() - getIntFromTOS()); break;
			case BC_DMUL: pushDoubleToTOS(getDoubleFromTOS() * getDoubleFromTOS()); break;
			case BC_IMUL: pushIntToTOS(getIntFromTOS() * getIntFromTOS()); break;
			case BC_DDIV: pushDoubleToTOS(getDoubleFromTOS() / getDoubleFromTOS()); break;
			case BC_IDIV: pushIntToTOS(getIntFromTOS() / getIntFromTOS()); break;
			case BC_IMOD: pushIntToTOS(getIntFromTOS() % getIntFromTOS()); break;
			case BC_DNEG: pushDoubleToTOS(-getDoubleFromTOS()); break;
			case BC_INEG: pushIntToTOS(-getIntFromTOS()); break;
			case BC_IPRINT: _out << getIntFromTOS(); break;
			case BC_DPRINT:_out << getDoubleFromTOS(); break;
			case BC_SPRINT: break;
			case BC_I2D: pushDoubleToTOS((double)getIntFromTOS()); break;
			case BC_D2I: pushIntToTOS((int)getDoubleFromTOS()); break;
			case BC_S2I: break;
			case BC_SWAP: break;
			case BC_POP: _stack.pop(); break;
			case BC_LOADDVAR0: loadDoubleVar(0); break;
			case BC_LOADDVAR1: loadDoubleVar(1); break;
			case BC_LOADDVAR2: loadDoubleVar(2); break;
			case BC_LOADDVAR3: loadDoubleVar(3); break;
			case BC_LOADIVAR0: loadIntVar(0); break;
			case BC_LOADIVAR1: loadIntVar(1); break;
			case BC_LOADIVAR2: loadIntVar(2); break;
			case BC_LOADIVAR3: loadIntVar(3); break;
			case BC_LOADSVAR0: break;
			case BC_LOADSVAR1: break;
			case BC_LOADSVAR2: break;
			case BC_LOADSVAR3: break;
			case BC_STOREDVAR0: storeDoubleVar(0); break;
			case BC_STOREDVAR1: storeDoubleVar(1); break;
			case BC_STOREDVAR2: storeDoubleVar(2); break;
			case BC_STOREDVAR3: storeDoubleVar(3); break;
			case BC_STOREIVAR0: storeIntVar(0); break;
			case BC_STOREIVAR1: storeIntVar(1); break;
			case BC_STOREIVAR2: storeIntVar(2); break;
			case BC_STOREIVAR3: storeIntVar(3); break;
			case BC_STORESVAR0: break;
			case BC_STORESVAR1: break;
			case BC_STORESVAR2: break;
			case BC_STORESVAR3: break;
			case BC_LOADDVAR: loadDoubleVar(readUInt16FromBytecode()); break;
			case BC_LOADIVAR: loadIntVar(readUInt16FromBytecode()); break;
			case BC_LOADSVAR: break;
			case BC_STOREDVAR: storeDoubleVar(readUInt16FromBytecode()); break;
			case BC_STOREIVAR: storeIntVar(readUInt16FromBytecode());break;
			case BC_STORESVAR: break;
			case BC_LOADCTXDVAR: break;
			case BC_LOADCTXIVAR: break;
			case BC_LOADCTXSVAR: break;
			case BC_STORECTXDVAR: break;
			case BC_STORECTXIVAR: break;
			case BC_STORECTXSVAR: break;
			case BC_DCMP: break;
			case BC_ICMP: break;
			case BC_JA: jump(); break;
			case BC_IFICMPNE: if (getIntFromTOS() != getIntFromTOS()) jump(); break;
			case BC_IFICMPE: if (getIntFromTOS() == getIntFromTOS()) jump(); break;
			case BC_IFICMPG: if (getIntFromTOS() > getIntFromTOS()) jump(); break;
			case BC_IFICMPGE: if (getIntFromTOS() >= getIntFromTOS()) jump(); break;
			case BC_IFICMPL: if (getIntFromTOS() < getIntFromTOS()) jump(); break;
			case BC_IFICMPLE: if (getIntFromTOS() <= getIntFromTOS()) jump(); break;
			case BC_DUMP: break;
			case BC_STOP: /*TODO: some cleanup here?*/return 0;
			case BC_CALL: break;
			case BC_CALLNATIVE: break;
			case BC_RETURN: break;
			case BC_BREAK: break;
			default: return new Status("unknown byte");
		}
	}
	return 0;
}

} /* namespace mathvm */
