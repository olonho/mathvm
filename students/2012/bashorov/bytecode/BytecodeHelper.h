#ifndef BYTECODE_HELPER_H
#define BYTECODE_HELPER_H

#include <stack>
#include "mathvm.h"

namespace mathvm {

class BytecodeHelper
{
	Code* _code;
	Bytecode* _bc;
	std::stack<VarType> _elTypes;

	void pushType(VarType type) { _elTypes.push(type); }
	void popType() { _elTypes.pop(); }
	VarType topType() { return _elTypes.top(); }
public:
	BytecodeHelper(Code* code): _code(code), _bc(0) {}

	BytecodeHelper& operator()(Bytecode* bc) {
		_bc = bc;
		return *this;
	}

	BytecodeHelper& invalid() {
		_bc->addInsn(BC_INVALID);
		return *this;
	}

	BytecodeHelper& load(const Var var) {
		if (var.type() == VT_INT)
			load(var.getIntValue());
		if (var.type() == VT_DOUBLE)
			load(var.getDoubleValue());
		if (var.type() == VT_STRING)
			load(std::string(var.getStringValue()));

		return *this;
	}

	BytecodeHelper& load(int64_t val) {
		if (val == 0) {
			_bc->addInsn(BC_ILOAD0);
		} else if (val == 1) {
			_bc->addInsn(BC_ILOAD1);
		} else if (val == -1) {
			_bc->addInsn(BC_ILOADM1);
		} else {
			_bc->addInsn(BC_ILOAD);
			_bc->addInt64(val);
		}

		pushType(VT_INT);
		return *this;
	}

	BytecodeHelper& load(double val) {
		if (val == 0.0) {
			_bc->addInsn(BC_DLOAD0);
		} else if (val == 1.0) {
			_bc->addInsn(BC_DLOAD1);
		} else if (val == -1.0) {
			_bc->addInsn(BC_DLOADM1);
		} else {
			_bc->addInsn(BC_DLOAD);
			_bc->addDouble(val);
		}

		pushType(VT_DOUBLE);
		return *this;
	}

	BytecodeHelper& load(const std::string& val) {
		if (val.empty()) {
			_bc->addInsn(BC_SLOAD0);
		} else {
			_bc->addInsn(BC_SLOAD);
			_bc->addUInt16(_code->makeStringConstant(val));
		}

		pushType(VT_STRING);
		return *this;
	}

	BytecodeHelper& print() {
		VarType type = topType();
		if (type == VT_INT)
			_bc->addInsn(BC_IPRINT);
		if (type == VT_DOUBLE)
			_bc->addInsn(BC_DPRINT);
		if (type == VT_STRING)
			_bc->addInsn(BC_SPRINT);

		popType();
		return *this;
	}

	#define POP0 
	#define POP1 popType();
	#define POP2 popType(); popType();

	#define PUSH0(IGNORE)
	#define PUSH1(type) pushType(VT_##type);

	#define SIMPLE(name, upper, POP, PUSH) \
	BytecodeHelper& name() { \
		_bc->addInsn(BC_##upper); \
		POP; \
		PUSH; \
		return *this;\
	}

	#define SIMPLE_DI(name, upper, POP, PUSH) \
		SIMPLE(d##name, D##upper, POP, PUSH(DOUBLE)) \
		SIMPLE(i##name, I##upper, POP, PUSH(INT))

	SIMPLE_DI(add, ADD, POP2, PUSH1)
	SIMPLE_DI(sub, SUB, POP2, PUSH1)
	SIMPLE_DI(mul, MUL, POP2, PUSH1)
	SIMPLE_DI(div, DIV, POP2, PUSH1)
	SIMPLE(imod, IMOD, POP2, PUSH1(INT))
	SIMPLE_DI(neg, NEG, POP1, PUSH1)
	SIMPLE(i2d, I2D, POP1, PUSH1(DOUBLE))
	SIMPLE(d2i, D2I, POP1, PUSH1(INT))
	SIMPLE(s2i, S2I, POP1, PUSH1(INT))

	#define LOADVAR(type, TYPE, PUSHTYPE) \
	BytecodeHelper& load##type##var(uint16_t id) { \
		switch (id) { \
			case 0: \
				_bc->addInsn(BC_LOAD##TYPE##VAR0); \
			break; \
			case 1: \
				_bc->addInsn(BC_LOAD##TYPE##VAR1); \
			break; \
			case 2: \
				_bc->addInsn(BC_LOAD##TYPE##VAR2); \
			break; \
			case 3: \
				_bc->addInsn(BC_LOAD##TYPE##VAR3); \
			break; \
			default: \
				_bc->addInsn(BC_LOAD##TYPE##VAR); \
				_bc->addUInt16(id); \
		} \
		pushType(VT_##PUSHTYPE);\
		return *this; \
	}

	#define LOADCTXVAR(type, TYPE, PUSHTYPE) \
	BytecodeHelper& load##type##ctxvar(uint16_t ctx, uint16_t id) { \
		_bc->addInsn(BC_LOADCTX##TYPE##VAR); \
		_bc->addUInt16(ctx); \
		_bc->addUInt16(id); \
		pushType(VT_##PUSHTYPE); \
		return *this; \
	}

	LOADVAR(d, D, DOUBLE)
	LOADVAR(i, I, INT)
	LOADVAR(s, S, STRING)
	LOADCTXVAR(d, D, DOUBLE)
	LOADCTXVAR(i, I, INT)
	LOADCTXVAR(s, S, STRING)

	#define PUSH_INT(IGNORE) pushType(VT_INT)
	SIMPLE_DI(cmp, CMP, POP2, PUSH_INT)

	BytecodeHelper& jmp(int16_t offset) {
		_bc->addInsn(BC_JA);
		_bc->addInt16(offset);
		return *this;
	}

	#define IFICMP(COND) \
	BytecodeHelper& ificmp##COND(int16_t offset) { \
		_bc->addInsn(BC_IFICMP##COND); \
		_bc->addInt16(offset); \
		POP2 \
		return *this; \
	}

	IFICMP(NE)
	IFICMP(E)
	IFICMP(G)
	IFICMP(GE)
	IFICMP(L)
	IFICMP(LE)

	SIMPLE(dump, DUMP, POP0, PUSH0(0))
	SIMPLE(stop, STOP, POP0, PUSH0(0))

	SIMPLE(ret, RETURN, POP0, PUSH0(0))
	SIMPLE(brk, BREAK, POP0, PUSH0(0))

	#define CMD_ID(cmd, CMD) \
	BytecodeHelper& cmd(uint16_t id) { \
		_bc->addInsn(BC_##CMD); \
		_bc->addUInt16(id); \
		return *this; \
	}

	CMD_ID(call, CALL)
	CMD_ID(callnative, CALLNATIVE)
};

//pop
//swap
//store__
}

#endif