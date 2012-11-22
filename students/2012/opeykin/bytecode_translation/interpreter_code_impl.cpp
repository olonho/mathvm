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

double InterpreterCodeImpl::nextDouble() {
	double result = _bp->getDouble(_ip);
	_ip += sizeof(double);
	return result;
}

int64_t InterpreterCodeImpl::popInt() {
	int64_t value = this->_stack.top()._intValue;
	_stack.pop();
	return value;
}

double InterpreterCodeImpl::popDouble() {
	double value = _stack.top()._doubleValue;
	_stack.pop();
	return value;
}

uint16_t InterpreterCodeImpl::popStringId() {
	uint16_t id = _stack.top()._stringId;
	_stack.pop();
	return id;
}

void InterpreterCodeImpl::pushInt(int value) {
	ContextVar unit;
	unit._intValue = value;
	_stack.push(unit);
}

void InterpreterCodeImpl::pushDouble(double value) {
	ContextVar unit;
	unit._doubleValue = value;
	_stack.push(unit);
}

void InterpreterCodeImpl::pushString(uint16_t id) {
	ContextVar unit;
	unit._stringId = id;
	_stack.push(unit);
}

void InterpreterCodeImpl::loadVar(uint32_t index) {
	_stack.push(*_context->getVar(index));
}

//void InterpreterCodeImpl::loadVar(uint32_t index) {
//	_stack.push(*_context->getVar(index));
//}

void InterpreterCodeImpl::storeIntVar(uint32_t index) {
	ContextVar* var = _context->getVar(index);
	var->_intValue = popInt();
}

void InterpreterCodeImpl::storeDoubleVar(uint32_t index) {
	ContextVar* var = _context->getVar(index);
	var->_doubleValue = popDouble();
}

void InterpreterCodeImpl::storeStringVar(uint32_t index) {
	ContextVar* var = _context->getVar(index);
	var->_stringId = popStringId();
}

void InterpreterCodeImpl::callFunction(uint32_t id) {
	if (_context != 0) {
		_context->setIp(_ip);
	}

	BytecodeFunction* function = bytecodeFunctionById(id);
	_context = new Context(_context, function);
	_bp = function->bytecode();
	_ip = 0;
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
			case BC_DLOAD: pushDouble(nextDouble()); break;
			case BC_ILOAD: pushInt(nextInt()); break;
			case BC_SLOAD: pushString(nextUInt16()); break;
			case BC_DLOAD0: pushDouble(0.0); break;
			case BC_ILOAD0: pushInt(0); break;
			case BC_SLOAD0: break;
			case BC_DLOAD1: pushDouble(1.0); break;
			case BC_ILOAD1: pushInt(1); break;
			case BC_DLOADM1: pushDouble(-1.0); break;
			case BC_ILOADM1: pushInt(-1); break;
			case BC_DADD: pushDouble(popDouble() + popDouble()); break;
			case BC_IADD: pushInt(popInt() + popInt()); break;
			case BC_DSUB: pushDouble(popDouble() - popDouble()); break;
			case BC_ISUB: pushInt(popInt() - popInt()); break;
			case BC_DMUL: pushDouble(popDouble() * popDouble()); break;
			case BC_IMUL: pushInt(popInt() * popInt()); break;
			case BC_DDIV: pushDouble(popDouble() / popDouble()); break;
			case BC_IDIV: pushInt(popInt() / popInt()); break;
			case BC_IMOD: pushInt(popInt() % popInt()); break;
			case BC_DNEG: pushDouble(-popDouble()); break;
			case BC_INEG: pushInt(-popInt()); break;
			case BC_IPRINT: _out << popInt() << endl; break;
			case BC_DPRINT: _out << popDouble() << endl; break;
			case BC_SPRINT: _out << constantById(popStringId()); break;
			case BC_I2D: pushDouble((double)popInt()); break;
			case BC_D2I: pushInt((int)popDouble()); break;
			case BC_S2I: break;
			case BC_SWAP: break;
			case BC_POP: _stack.pop(); break;
			case BC_LOADDVAR0: loadVar(0); break;
			case BC_LOADDVAR1: loadVar(1); break;
			case BC_LOADDVAR2: loadVar(2); break;
			case BC_LOADDVAR3: loadVar(3); break;
			case BC_LOADIVAR0: loadVar(0); break;
			case BC_LOADIVAR1: loadVar(1); break;
			case BC_LOADIVAR2: loadVar(2); break;
			case BC_LOADIVAR3: loadVar(3); break;
			case BC_LOADSVAR0: loadVar(0); break;
			case BC_LOADSVAR1: loadVar(1); break;
			case BC_LOADSVAR2: loadVar(2); break;
			case BC_LOADSVAR3: loadVar(3); break;
			case BC_STOREDVAR0: storeDoubleVar(0); break;
			case BC_STOREDVAR1: storeDoubleVar(1); break;
			case BC_STOREDVAR2: storeDoubleVar(2); break;
			case BC_STOREDVAR3: storeDoubleVar(3); break;
			case BC_STOREIVAR0: storeIntVar(0); break;
			case BC_STOREIVAR1: storeIntVar(1); break;
			case BC_STOREIVAR2: storeIntVar(2); break;
			case BC_STOREIVAR3: storeIntVar(3); break;
			case BC_STORESVAR0: storeStringVar(0); break;
			case BC_STORESVAR1: storeStringVar(1); break;
			case BC_STORESVAR2: storeStringVar(2); break;
			case BC_STORESVAR3: storeStringVar(3); break;
			case BC_LOADDVAR: loadVar(nextUInt16()); break;
			case BC_LOADIVAR: loadVar(nextUInt16()); break;
			case BC_LOADSVAR: loadVar(nextUInt16()); break;
			case BC_STOREDVAR: storeDoubleVar(nextUInt16()); break;
			case BC_STOREIVAR: storeIntVar(nextUInt16());break;
			case BC_STORESVAR: storeStringVar(nextUInt16()); break;
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
