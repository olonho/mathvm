#include "BytecodeInterpreter.h"
#include <iostream>

using namespace mathvm;

BytecodeInterpreter::BytecodeInterpreter(Code *code) : _code(code), _insnPtr(0), _shouldReturn(false)
{
	_dataStack = new DataStack();
	_stackFrame = new FuncStackFrame();
}


BytecodeInterpreter::~BytecodeInterpreter(void)
{
	delete _stackFrame;
	delete _dataStack;
}

void BytecodeInterpreter::jump(int32_t offset) {
	_insnPtr += offset;
}

void BytecodeInterpreter::initFuncParameters() {
	int countParams = _stackFrame->getCurFrame()->_countParams;
	for (int i = countParams; i >=1; --i) {
		_stackFrame->updateVar(i, _dataStack->pop());
	}
}

Status* BytecodeInterpreter::call(uint16_t id) {
	//save previous execute environment
	if (!_stackFrame->isEmpty()) _stackFrame->getCurFrame()->_insnPtr = _insnPtr;
	Bytecode* prevBytecode = _bytecode;
	//initialize new execute environment
	TranslatedFunction * callFunc = _code->functionById(id); 
	_stackFrame->addFrame(callFunc);
	if (callFunc->parametersNumber() && !_dataStack->isEmpty())initFuncParameters();
	_bytecode = static_cast<BytecodeFunction *>(callFunc)->bytecode();
	_insnPtr = 0;
	//execute
	uint32_t bytecodeSize = _bytecode->length();
	Status insnStatus;
	while (_insnPtr < bytecodeSize && !_shouldReturn) {
		Instruction insn = _bytecode->getInsn(_insnPtr++);
		insnStatus = processInsn(insn);
		if (insnStatus.isError()) break;
	}
	if (!_stackFrame->isEmpty()) {
		_bytecode = prevBytecode;
		_insnPtr = _stackFrame->getCurFrame()->_insnPtr;
	}
	if (_shouldReturn) _shouldReturn = false;
	return new Status(insnStatus);
}

Status BytecodeInterpreter::processInsn(Instruction insn) {
	switch (insn)
	{
	case BC_INVALID:
		return Status("INVALID instruction"); break;
	case BC_DLOAD: //"Load double on TOS, inlined into insn stream."
		{
			double value = _bytecode->getDouble(_insnPtr);
			_insnPtr += 8;
			_dataStack->push(StackValue(value));
			break;
		}
	case BC_ILOAD: //"Load int on TOS, inlined into insn stream." 
		{
			int64_t value = _bytecode->getInt64(_insnPtr);
			_insnPtr += 8;
			_dataStack->push(StackValue(value));
			break;
		}
	case BC_SLOAD: //"Load string reference on TOS, next two bytes - constant id."
		{
			uint16_t strId = getNextUInt16();
			_dataStack->push(StackValue(_code->constantById(strId).c_str()));
			break;
		}
	case BC_DLOAD0: //"Load double 0 on TOS."
		_dataStack->push(StackValue((double)0));
		break;
	case BC_ILOAD0: //"Load int 0 on TOS."
		_dataStack->push(StackValue((int64_t)0));
		break;
	case BC_SLOAD0: //"Load empty string on TOS."
		//??????
		break;
	case BC_DLOAD1: //"Load double 1 on TOS."
		_dataStack->push(StackValue((double)1));
		break;
	case BC_ILOAD1: //"Load int 1 on TOS."
		_dataStack->push(StackValue((int64_t)1));
		break;
	case BC_DLOADM1: //"Load double -1 on TOS."
		_dataStack->push(StackValue((double)-1));
		break;
	case BC_ILOADM1: //"Load int -1 on TOS."
		_dataStack->push(StackValue((int64_t)-1));
		break;
	case BC_DADD: //"Add 2 doubles on TOS, push value back."
		_dataStack->push(StackValue(getTOSDouble()+getTOSDouble()));
		break;
	case BC_IADD: //"Add 2 ints on TOS, push value back."
		_dataStack->push(StackValue(getTOSInt()+getTOSInt()));
		break;
	case BC_DSUB: //"Subtract 2 doubles on TOS (lower from upper), push value back."
		{
			double val1 = getTOSDouble();
			double val2 = getTOSDouble();
			_dataStack->push(StackValue(val1-val2));
			break;
		}
	case BC_ISUB: //"Subtract 2 ints on TOS (lower from upper), push value back."
		{
			int64_t val1 = getTOSInt();
			int64_t val2 = getTOSInt();
			_dataStack->push(StackValue(val1-val2));
			break;
		}
	case BC_DMUL: //"Multiply 2 doubles on TOS, push value back."
		_dataStack->push(StackValue(getTOSDouble()*getTOSDouble()));
		break;
	case BC_IMUL: //"Multiply 2 ints on TOS, push value back."
		_dataStack->push(StackValue(getTOSInt()*getTOSInt()));
		break;
	case BC_DDIV: //"Divide 2 doubles on TOS (upper to lower), push value back." 
		{
			double val1 = getTOSDouble();
			double val2 = getTOSDouble();
			_dataStack->push(StackValue(val1/val2));
			break;
		}
	case BC_IDIV:  //"Divide 2 ints on TOS (upper to lower), push value back."
		{
			int64_t val1 = getTOSInt();
			int64_t val2 = getTOSInt();
			_dataStack->push(StackValue(val1/val2));
			break;
		}
	case BC_IMOD: //"Modulo operation on 2 ints on TOS (upper to lower), push value back."
		{
			int64_t val1 = getTOSInt();
			int64_t val2 = getTOSInt();
			_dataStack->push(StackValue((int64_t)(val1 % val2)));
			break;
		}
	case BC_DNEG: //"Negate double on TOS."
		_dataStack->push(StackValue(-getTOSDouble()));
		break;
	case BC_INEG: //"Negate int on TOS."
		_dataStack->push(StackValue(-getTOSInt()));
		break;
	case BC_IPRINT: //"Pop and print integer TOS."
		std::cout << getTOSInt();
		break;
	case BC_DPRINT: //"Pop and print double TOS."
		std::cout << getTOSDouble();
		break;
	case BC_SPRINT: //"Pop and print string TOS."
		std::cout << getTOSStr();
		break;
	case BC_I2D:  //"Convert int on TOS to double."
		_dataStack->push(StackValue((double)getTOSInt()));
		break;
	case BC_D2I: //"Convert double on TOS to int."
		_dataStack->push(StackValue((int64_t)getTOSDouble()));
		break;
	case BC_SWAP: //"Swap 2 topmost values."
		{
			StackValue val1 = _dataStack->pop();
			StackValue val2 = _dataStack->pop();
			_dataStack->push(val1);
			_dataStack->push(val2);
			break;
		}
	case BC_POP: //"Remove topmost value."
		_dataStack->pop();
		break;
	case BC_LOADDVAR0: //"Load double from variable 0, push on TOS."
		{
			double val = _stackFrame->getVar(0)._doubleValue;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_LOADDVAR1: //"Load double from variable 1, push on TOS."
		{
			double val = _stackFrame->getVar(1)._doubleValue;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_LOADDVAR2: //"Load double from variable 2, push on TOS."
		{
			double val = _stackFrame->getVar(2)._doubleValue;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_LOADDVAR3: //"Load double from variable 3, push on TOS."
		{
			double val = _stackFrame->getVar(3)._doubleValue;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_LOADIVAR0: //"Load int from variable 0, push on TOS."
		{
			int64_t val = _stackFrame->getVar(0)._doubleValue;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_LOADIVAR1: //"Load int from variable 1, push on TOS."
		{
			int64_t val = _stackFrame->getVar(1)._doubleValue;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_LOADIVAR2: //"Load int from variable 2, push on TOS."
		{
			int64_t val = _stackFrame->getVar(2)._doubleValue;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_LOADIVAR3: //"Load int from variable 3, push on TOS."
		{
			int64_t val = _stackFrame->getVar(3)._doubleValue;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_LOADSVAR0: //"Load string from variable 0, push on TOS."
		{
			const char* val = _stackFrame->getVar(0)._strPtr;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_LOADSVAR1: //"Load string from variable 1, push on TOS."
		{
			const char* val = _stackFrame->getVar(1)._strPtr;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_LOADSVAR2: //"Load string from variable 2, push on TOS."
		{
			const char* val = _stackFrame->getVar(2)._strPtr;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_LOADSVAR3: //"Load string from variable 3, push on TOS."
		{
			const char* val = _stackFrame->getVar(3)._strPtr;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_STOREDVAR0: //"Pop TOS and store to double variable 0."
		_stackFrame->updateVar(0, StackValue(getTOSDouble()));
		break;	
	case BC_STOREDVAR1: //"Pop TOS and store to double variable 1."
		_stackFrame->updateVar(1, StackValue(getTOSDouble()));
		break;
	case BC_STOREDVAR2: //"Pop TOS and store to double variable 0."
		_stackFrame->updateVar(2, StackValue(getTOSDouble()));
		break;
	case BC_STOREDVAR3: //"Pop TOS and store to double variable 3."
		_stackFrame->updateVar(3, StackValue(getTOSDouble()));
		break;
	case BC_STOREIVAR0: //"Pop TOS and store to int variable 0."
		_stackFrame->updateVar(0, StackValue(getTOSInt()));
		break;
	case BC_STOREIVAR1: //"Pop TOS and store to int variable 1."
		_stackFrame->updateVar(1, StackValue(getTOSInt()));
		break;
	case BC_STOREIVAR2: //"Pop TOS and store to int variable 2."
		_stackFrame->updateVar(2, StackValue(getTOSInt()));
		break;
	case BC_STOREIVAR3: //"Pop TOS and store to int variable 3."
		_stackFrame->updateVar(3, StackValue(getTOSInt()));
		break;
	case BC_STORESVAR0: //"Pop TOS and store to string variable 0."
		_stackFrame->updateVar(0, StackValue(getTOSStr()));
		break;
	case BC_STORESVAR1: //"Pop TOS and store to string variable 1."
		_stackFrame->updateVar(1, StackValue(getTOSStr()));
		break;
	case BC_STORESVAR2: //"Pop TOS and store to string variable 2."
		_stackFrame->updateVar(2, StackValue(getTOSStr()));
		break;
	case BC_STORESVAR3: //"Pop TOS and store to string variable 3."
		_stackFrame->updateVar(3, StackValue(getTOSStr()));
		break;
	case BC_LOADDVAR: //"Load double from variable, whose 2-byte is id inlined to insn stream, push on TOS."
		{
			double val = _stackFrame->getVar(getNextUInt16())._doubleValue;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_LOADIVAR: //"Load int from variable, whose 2-byte id is inlined to insn stream, push on TOS."
		{
			int64_t val = _stackFrame->getVar(getNextUInt16())._intValue;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_LOADSVAR: //"Load string from variable, whose 2-byte id is inlined to insn stream, push on TOS."
		{
			const char* val = _stackFrame->getVar(getNextUInt16())._strPtr;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_STOREDVAR: //"Pop TOS and store to double variable, whose 2-byte id is inlined to insn stream."
		_stackFrame->updateVar(getNextUInt16(), StackValue(getTOSDouble()));
		break;
	case BC_STOREIVAR: //"Pop TOS and store to int variable, whose 2-byte id is inlined to insn stream."
		_stackFrame->updateVar(getNextUInt16(), StackValue(getTOSInt()));
		break;
	case BC_STORESVAR: //"Pop TOS and store to string variable, whose 2-byte id is inlined to insn stream."
		_stackFrame->updateVar(getNextUInt16(), StackValue(getTOSStr()));
		break;
	case BC_LOADCTXDVAR: //"Load double from variable, whose 2-byte context and 2-byte id inlined to insn stream, push on TOS."
		{
			uint16_t ctx = getNextUInt16();
			uint16_t id = getNextUInt16();
			double val = _stackFrame->getVar(ctx, id)._doubleValue;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_LOADCTXIVAR: //"Load int from variable, whose 2-byte context and 2-byte id is inlined to insn stream, push on TOS."
		{
			uint16_t ctx = getNextUInt16();
			uint16_t id = getNextUInt16();
			int64_t val = _stackFrame->getVar(ctx, id)._intValue;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_LOADCTXSVAR: //"Load string from variable, whose 2-byte context and 2-byte id is inlined to insn stream, push on TOS."
		{
			uint16_t ctx = getNextUInt16();
			uint16_t id = getNextUInt16();
			const char* val = _stackFrame->getVar(ctx, id)._strPtr;
			_dataStack->push(StackValue(val));
			break;
		}
	case BC_STORECTXDVAR: //"Pop TOS and store to double variable, whose 2-byte context and 2-byte id is inlined to insn stream."
		{
			uint16_t ctx = getNextUInt16();
			uint16_t id = getNextUInt16();
			double val = getTOSDouble();
			_stackFrame->updateVar(ctx, id, StackValue(val));
			break;
		}
	case BC_STORECTXIVAR: //"Pop TOS and store to int variable, whose 2-byte context and 2-byte id is inlined to insn stream."
		{
			uint16_t ctx = getNextUInt16();
			uint16_t id = getNextUInt16();
			int64_t val = getTOSInt();
			_stackFrame->updateVar(ctx, id, StackValue(val));
			break;
		}
	case BC_STORECTXSVAR: //"Pop TOS and store to string variable, whose 2-byte context and 2-byte id is inlined to insn stream."
		{
			uint16_t ctx = getNextUInt16();
			uint16_t id = getNextUInt16();
			const char* val = getTOSStr();
			_stackFrame->updateVar(ctx, id, StackValue(val));
			break;
		}
	case BC_DCMP: //"Compare 2 topmost doubles, pushing libc-stryle comparator value cmp(upper, lower) as integer."
		{
			double val1 = getTOSDouble();
			double val2 = getTOSDouble();
			_dataStack->push(StackValue(cmp(val1, val2)));
			break;
		}
	case BC_ICMP: //"Compare 2 topmost ints, pushing libc-style comparator value cmp(upper, lower) as integer."
		{
			int64_t val1 = getTOSInt();
			int64_t val2 = getTOSInt();
			_dataStack->push(StackValue(cmp(val1, val2)));
			break;
		}
	case BC_JA: //"Jump always, next two bytes - signed offset of jump destination."
		jump(getOffset());
		break;
	case BC_IFICMPNE: //"Compare two topmost integers and jump if upper != lower, next two bytes - signed offset of jump destination."
		{
			int64_t val1 = getTOSInt();
			int64_t val2 = getTOSInt();
			(cmp(val1, val2)) ? jump(getOffset()) : _insnPtr +=2;
			break;
		}
			
	case BC_IFICMPE: //"Compare two topmost integers and jump if upper == lower, next two bytes - signed offset of jump destination."
		{
			int64_t val1 = getTOSInt();
			int64_t val2 = getTOSInt();
			(!cmp(val1, val2)) ? jump(getOffset()) : _insnPtr +=2;
			break;
		}
	case BC_IFICMPG: //"Compare two topmost integers and jump if upper > lower, next two bytes - signed offset of jump destination."
		{
			int64_t val1 = getTOSInt();
			int64_t val2 = getTOSInt();
			(cmp(val1, val2) == 1) ? jump(getOffset()) : _insnPtr +=2;
			break;
		}
		
	case BC_IFICMPGE: //"Compare two topmost integers and jump if upper >= lower, next two bytes - signed offset of jump destination."
		{
			int64_t val1 = getTOSInt();
			int64_t val2 = getTOSInt();
			(cmp(val1, val2) != -1) ? jump(getOffset()) : _insnPtr +=2;
			break;
		}
		
	case BC_IFICMPL: //"Compare two topmost integers and jump if upper < lower, next two bytes - signed offset of jump destination."
		{
			int64_t val1 = getTOSInt();
			int64_t val2 = getTOSInt();
			(cmp(val1, val2) == -1) ? jump(getOffset()) : _insnPtr +=2;
			break;
		}
	case BC_IFICMPLE: //"Compare two topmost integers and jump if upper <= lower, next two bytes - signed offset of jump destination."
		{
			int64_t val1 = getTOSInt();
			int64_t val2 = getTOSInt();
			(cmp(val1, val2) != 1) ? jump(getOffset()) : _insnPtr +=2;
			break;
		}
	case BC_DUMP: //"Dump value on TOS, without removing it."
		//????????????????
		break;
	case BC_STOP: //"Stop execution."
		_stackFrame->removeTop();
		break;
	case BC_CALL: //"Call function, next two bytes - unsigned function id."
		call(getNextUInt16());
		break;
	case BC_CALLNATIVE: //"Call native function, next two bytes - id of the native function."
		jump(2);
		break;
	case BC_RETURN: //"Return to call location"
		_stackFrame->removeTop();
		_shouldReturn = true;
		break;
    default:
		break;
	}
	return Status();
}

uint16_t BytecodeInterpreter::getNextUInt16() {
	uint16_t val = _bytecode->getUInt16(_insnPtr);
	_insnPtr +=2;
	return val;
}

int16_t BytecodeInterpreter::getOffset() {
	int16_t val = _bytecode->getInt16(_insnPtr);
	return val;
}

int64_t BytecodeInterpreter::cmp(int64_t val1, int64_t val2) {
	int64_t res = 0;
	(val1 == val2) ? res = 0 : ((val1 > val2) ? res = 1 : res = -1);
	return res;	
}

int64_t BytecodeInterpreter::cmp(double val1, double val2) {
	int64_t res = 0;
	(val1 == val2) ? res = 0 : ((val1 > val2) ? res = 1 : res = -1);
	return res;
}

int64_t BytecodeInterpreter::getTOSInt() {
	return _dataStack->pop()._intValue;
}

double BytecodeInterpreter::getTOSDouble() {
	return _dataStack->pop()._doubleValue;
}

const char* BytecodeInterpreter::getTOSStr() {
	return _dataStack->pop()._strPtr;
}
