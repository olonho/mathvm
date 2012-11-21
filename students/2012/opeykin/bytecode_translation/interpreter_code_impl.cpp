/*
 * interpreter_code_impl.cpp
 *
 *  Created on: Nov 20, 2012
 *      Author: alex
 */

#include "interpreter_code_impl.h"

namespace mathvm {

InterpreterCodeImpl::InterpreterCodeImpl(ostream& out) : _out(out) {
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

uint16_t InterpreterCodeImpl::nextUInt16() {
	uint16_t result = _bp->getUInt16(_ip);
	_ip += sizeof(uint16_t);
	return result;
}

int64_t InterpreterCodeImpl::nextInt() {
	int64_t result = _bp->getInt64(_ip);
	_ip += sizeof(int64_t);
	return result;
}

double InterpreterCodeImpl::readDoubleFromBytecode() {
	double result = _bp->getDouble(_ip);
	_ip += sizeof(double);
	return result;
}

int64_t InterpreterCodeImpl::getIntFromTOS() {
	int64_t value = this->_stack.top()._intValue;
	_stack.pop();
	return value;
}

double InterpreterCodeImpl::getDoubleFromTOS() {
	double value = _stack.top()._doubleValue;
	_stack.pop();
	return value;
}

void InterpreterCodeImpl::pushIntToTOS(int value) {
	ContextVar unit;
	unit._intValue = value;
	_stack.push(unit);
}

void InterpreterCodeImpl::pushDoubleToTOS(double value) {
	ContextVar unit;
	unit._doubleValue = value;
	_stack.push(unit);
}

void InterpreterCodeImpl::loadIntVar(uint32_t index) {
	_stack.push(*_context->getVar(index));
}

void InterpreterCodeImpl::loadDoubleVar(uint32_t index) {
	_stack.push(*_context->getVar(index));
}

void InterpreterCodeImpl::storeIntVar(uint32_t index) {
	ContextVar* var = _context->getVar(index);
	var->_intValue = getIntFromTOS();
}

void InterpreterCodeImpl::storeDoubleVar(uint32_t index) {
	ContextVar* var = _context->getVar(index);
	var->_doubleValue = getDoubleFromTOS();
}

void InterpreterCodeImpl::callFunction(uint32_t id) {
	if (_context != 0) {
		_context->setIp(_ip);
	}

	BytecodeFunction* function = bytecodeFunctionById(id);
	_context = new Context(_context, function);
	_bp = function->bytecode();
	_ip = 0;
//	for (uint16_t i = 0; i < function->parametersNumber(); ++i) {
//
//	}
}

void InterpreterCodeImpl::returnFromFunction() {
	Context* tmp = _context;
	_context = _context->parent();
	delete tmp;
	_ip = _context->ip();
	_bp = _context->bytecode();
}

Status* InterpreterCodeImpl::execute(vector<Var*>& vars) {
	callFunction(0); // top function

	while (true) {
		Instruction instruction = _bp->getInsn(_ip);
		_ip += sizeof(int8_t);
		switch(instruction) {
			case BC_INVALID: return new Status("Invalid instruction"); break;
			case BC_DLOAD: pushDoubleToTOS(readDoubleFromBytecode()); break;
			case BC_ILOAD: pushIntToTOS(nextInt()); break;
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
			case BC_IPRINT: _out << getIntFromTOS() << endl; break;
			case BC_DPRINT:_out << getDoubleFromTOS() << endl; break;
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
			case BC_LOADDVAR: loadDoubleVar(nextUInt16()); break;
			case BC_LOADIVAR: loadIntVar(nextUInt16()); break;
			case BC_LOADSVAR: break;
			case BC_STOREDVAR: storeDoubleVar(nextUInt16()); break;
			case BC_STOREIVAR: storeIntVar(nextUInt16());break;
			case BC_STORESVAR: break;
			case BC_LOADCTXDVAR: break;
			case BC_LOADCTXIVAR: break;
			case BC_LOADCTXSVAR: break;
			case BC_STORECTXDVAR: break;
			case BC_STORECTXIVAR: break;
			case BC_STORECTXSVAR: break;
			case BC_DCMP: break;
			case BC_ICMP: break;
			case BC_JA: _ip += _bp->getInt16(_ip); break;
			case BC_IFICMPNE: jump(not_equal_to<int64_t>()); break;
			case BC_IFICMPE: jump(equal_to<int64_t>());; break;
			case BC_IFICMPG: jump(greater<int64_t>()); break;
			case BC_IFICMPGE: jump(greater_equal<int64_t>()); break;
			case BC_IFICMPL: jump(less<int64_t>()); break;
			case BC_IFICMPLE: jump(less_equal<int64_t>()); break;
			case BC_DUMP: break;
			case BC_STOP: /*TODO: some cleanup here?*/return 0;
			case BC_CALL: callFunction(nextUInt16()); break;
			case BC_CALLNATIVE: break;
			case BC_RETURN: returnFromFunction(); break;
			case BC_BREAK: break;
			default: return new Status("unknown byte");
		}
	}
	return 0;
}

} /* namespace mathvm */
