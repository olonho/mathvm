#ifndef BYTECODE_HELPER_H
#define BYTECODE_HELPER_H

#include <stack>
#include "mathvm.h"

namespace mathvm {

class BytecodeHelper
{
	typedef std::map<std::string, uint16_t> variables2id_t;

	Code* _code;
	Bytecode* _bc;
	std::stack<VarType> _elTypes;
	variables2id_t _variables;

	void pushType(VarType type) { _elTypes.push(type); }
	void popType() { _elTypes.pop(); }
	VarType topType() { return _elTypes.top(); }
	VarType pretopType() {
		VarType t = _elTypes.top();
		_elTypes.pop();
		VarType ret = _elTypes.top();
		_elTypes.push(t);
		return ret;
	}
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

	BytecodeHelper& load(int val) { return load((int64_t) val); }

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

	#define SIMPLE2(name, NAME, POP, PUSH, PRE) \
	BytecodeHelper& name() { \
		PRE \
		VarType type = topType(); \
		if (type == VT_INT) \
			_bc->addInsn(BC_I##NAME); \
		if (type == VT_DOUBLE) \
			_bc->addInsn(BC_D##NAME); \
		POP \
		pushType(type); \
		return *this; \
	}

	#define DO_NOTHING
	#define CONVERT_IF_NEED {\
		VarType type1 = pretopType(); \
		VarType type2 = topType(); \
		if (type1 == VT_INT && type2 == VT_DOUBLE) { \
			swap(); \
			i2d(); \
		} else if (type1 == VT_DOUBLE && type2 == VT_INT) { \
			i2d(); \
		} \
	}

	#define SIMPLE2_BINOP(name, NAME, POP, PUSH) SIMPLE2(name, NAME, POP, PUSH, CONVERT_IF_NEED)

	#define SIMPLE_DI(name, upper, POP, PUSH) \
		SIMPLE(d##name, D##upper, POP, PUSH(DOUBLE)) \
		SIMPLE(i##name, I##upper, POP, PUSH(INT))

	SIMPLE2_BINOP(add, ADD, POP2, PUSH1)
	SIMPLE2_BINOP(sub, SUB, POP2, PUSH1)
	SIMPLE2_BINOP(mul, MUL, POP2, PUSH1)
	SIMPLE2_BINOP(div, DIV, POP2, PUSH1)
	SIMPLE2(neg, NEG, POP1, PUSH1, DO_NOTHING)

	SIMPLE(imod, IMOD, POP2, PUSH1(INT))
	SIMPLE(i2d, I2D, POP1, PUSH1(DOUBLE))
	SIMPLE(d2i, D2I, POP1, PUSH1(INT))
	SIMPLE(s2i, S2I, POP1, PUSH1(INT))

	#define LOWER_PREFIX(prefix, PREFIX, name, NAME) prefix
	#define UPPER_PREFIX(prefix, PREFIX, name, NAME) PREFIX
	#define LOWER_NAME(prefix, PREFIX, name, NAME) name
	#define UPPER_NAME(prefix, PREFIX, name, NAME) NAME
	#define DOUBLE(GETTER, a) GETTER(d, D, DOUBLE)
	#define INT(GETTER) GETTER(i, I, int, INT)
	#define STRING(GETTER) GETTER(s, S, string, STRING)

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

//	uint16_t varName2id(const string& name) {
//		variables2id_t::iterator it = _variables.find(name);
//		if (it == _variables.end()) {
//			it = _variables.insert(std::make_pair(name, _variables.size())).first;
//		}
//
//		return it->second;
//	}

	BytecodeHelper& loadvar(const AstVar* const var) {
		variables2id_t::iterator it = _variables.find(var->name());
		//todo что делать если переменной нет?
		assert (it != _variables.end());

		const uint16_t id = it->second;
		if (var->type() == VT_INT)
			loadivar(id);
		if (var->type() == VT_DOUBLE)
			loaddvar(id);
		if (var->type() == VT_STRING)
			loadsvar(id);

		return *this;
	}

	#define STOREVAR(type, TYPE, PUSHTYPE) \
	BytecodeHelper& store##type##var(uint16_t id) { \
		switch (id) { \
			case 0: \
				_bc->addInsn(BC_STORE##TYPE##VAR0); \
			break; \
			case 1: \
				_bc->addInsn(BC_STORE##TYPE##VAR1); \
			break; \
			case 2: \
				_bc->addInsn(BC_STORE##TYPE##VAR2); \
			break; \
			case 3: \
				_bc->addInsn(BC_STORE##TYPE##VAR3); \
			break; \
			default: \
				_bc->addInsn(BC_STORE##TYPE##VAR); \
				_bc->addUInt16(id); \
		} \
		POP1 \
		return *this; \
	}

	#define STORECTXVAR(type, TYPE, PUSHTYPE) \
	BytecodeHelper& store##type##ctxvar(uint16_t ctx, uint16_t id) { \
		_bc->addInsn(BC_STORECTX##TYPE##VAR); \
		_bc->addUInt16(ctx); \
		_bc->addUInt16(id); \
		POP1 \
		return *this; \
	}

	STOREVAR(d, D, DOUBLE)
	STOREVAR(i, I, INT)
	STOREVAR(s, S, STRING)
	STORECTXVAR(d, D, DOUBLE)
	STORECTXVAR(i, I, INT)
	STORECTXVAR(s, S, STRING)

	BytecodeHelper& storevar(const AstVar* const var) {
		assert(var != 0);

		variables2id_t::iterator it = _variables.find(var->name());
		if (it == _variables.end()) {
			it = _variables.insert(std::make_pair(var->name(), _variables.size())).first;
		}

		const uint16_t id = it->second;

		VarType type = topType();
		if (var->type() == VT_INT) {
			if (type == VT_DOUBLE) {
				d2i();
			} else if (type == VT_STRING) {
				s2i();
			}
			storeivar(id);
		}
		if (var->type() == VT_DOUBLE) {
			if (type == VT_INT) {
				i2d();
			}
			storedvar(id);
		}
		if (var->type() == VT_STRING) {
			assert(type == VT_STRING);
			storesvar(id);
		}

		return *this;
	}

	#define PUSH_INT(IGNORE) pushType(VT_INT)
	SIMPLE_DI(cmp, CMP, POP2, PUSH_INT)
	BytecodeHelper& cmp() {
		VarType upper = pretopType();
		VarType lower = topType();
		if (lower == VT_INT && upper == VT_INT) {
			icmp();
		} else {
			//todo extract to method
			if (lower == VT_INT) {
				i2d();
			}
			if (upper == VT_INT) {
				swap();
				i2d();
			}
			dcmp();
		}
		return *this;
	}

	BytecodeHelper& jmp(Label& label) {
		_bc->addBranch(BC_JA, label);
		return *this;
	}

	#define IFCMP(COND) \
	BytecodeHelper& ifcmp##COND(Label& label) { \
		VarType upper = pretopType(); \
		VarType lower = topType(); \
		Instruction instruction = BC_IFICMP##COND; \
		if (lower != VT_INT || upper != VT_INT) { \
		 	 if (lower == VT_INT) {\
		 		i2d(); \
		 	 } \
		 	 if (upper == VT_INT) {\
		 		swap(); \
		 		i2d(); \
		 		switch (instruction) { \
					case BC_IFICMPG: instruction = BC_IFICMPL; break; \
					case BC_IFICMPGE: instruction = BC_IFICMPLE; break; \
					case BC_IFICMPL: instruction = BC_IFICMPG; break; \
					case BC_IFICMPLE: instruction = BC_IFICMPGE; break; \
					default:; \
				} \
		 	 } \
			dcmp(); \
			load(0); \
		} \
		_bc->addBranch(instruction, label); \
		POP2 \
		return *this; \
	}

	IFCMP(NE)
	IFCMP(E)
	IFCMP(G)
	IFCMP(GE)
	IFCMP(L)
	IFCMP(LE)

	SIMPLE(dump, DUMP, POP0, PUSH0(0))
	SIMPLE(stop, STOP, POP0, PUSH0(0))

	SIMPLE(pop, RETURN, POP1, PUSH0(0))

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

	BytecodeHelper& swap() {
		_bc->addInsn(BC_SWAP);
		VarType type1 = pretopType();
		VarType type2 = topType();

		POP2
		pushType(type1);
		pushType(type2);

		return *this;
	}

	BytecodeHelper& inot() {
		Label ELSE;
		Label END;

		load(false);
		ifcmpNE(ELSE);

		load(true);
		jmp(END);

		setLabel(ELSE);
		load(false);

		setLabel(END);

		return *this;
	}

	BytecodeHelper& setLabel(Label& label) {
		label.bind(_bc->current(), _bc);
		return *this;
	}
};

}

#endif
