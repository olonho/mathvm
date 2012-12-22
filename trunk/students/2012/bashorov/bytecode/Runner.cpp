#include "Runner.h"
#include <iostream>
#include <sstream>

namespace mathvm {

namespace {

template <typename T>
struct Printer : std::unary_function<T, void> {
	void operator()(T val) {
		std::cout << val;
	}
};

template <typename T>
T id(FromStream*, T val) {
	return val;
}

template <typename T, int val>
T retVal() {
	return val;
}

const std::string emptyStr() {
	return "";
}

template <typename FROM, typename TO>
struct Caster : std::unary_function<FROM, TO> {
	TO operator()(FROM val) {
		return static_cast<TO>(val);
	}
};

template <>
struct Caster<std::string, int64_t> : std::unary_function<std::string, int64_t> {
	int64_t operator()(std::string val) {
		int64_t result = 0;
		std::stringstream(val) >> result;
		return result;
	}
};

template <typename T>
struct Comparator: std::binary_function<T, T, int64_t> {
	int64_t operator()(T left, T right) {
		return int64_t(left) - int64_t(right);
	}
};

template <typename T>
T getTypedValue(const Var& var);

template <>
double getTypedValue<double>(const Var& var) {
	assert(var.type() == VT_DOUBLE);
	return var.getDoubleValue();
}

template <>
int64_t getTypedValue<int64_t>(const Var& var) {
	assert(var.type() == VT_INT);
	return var.getIntValue();
}

template <>
std::string getTypedValue<std::string>(const Var& var) {
	assert(var.type() == VT_STRING);
	return var.getStringValue();
}

void setTypedValue(Var& var, double value) {
	assert(var.type() == VT_DOUBLE);
	var.setDoubleValue(value);
}

void setTypedValue(Var& var, int64_t value) {
	assert(var.type() == VT_INT);
	var.setIntValue(value);
}

void setTypedValue(Var& var, const std::string& value) {
	assert(var.type() == VT_STRING);
	var.setStringValue(value.c_str());
}

template<typename T>
VarType varType();
template<>
VarType varType<bool>() { return VT_INT; }
template<>
VarType varType<int>() { return VT_INT; }
template<>
VarType varType<int64_t>() { return VT_INT; }
template<>
VarType varType<double>() { return VT_DOUBLE; }
template<>
VarType varType<std::string>() { return VT_STRING; }

}

template <typename T, uint16_t id>
T Runner::loadvar() {
	std::map<uint16_t, Var>::iterator it = _state->variables.find(id);
	assert(it != _state->variables.end());
	return getTypedValue<T>(it->second);
}

template <typename T>
T Runner::loadvarById(uint16_t id) {
	std::map<uint16_t, Var>::iterator it = _state->variables.find(id);
	assert(it != _state->variables.end());
	return getTypedValue<T>(it->second);
}

template <typename T>
T Runner::loadvarById(uint16_t  ctx, uint16_t id) {
	State& ctxstate = _states[ctx].top();
	std::map<uint16_t, Var>::iterator it = ctxstate.variables.find(id);
	assert(it != ctxstate.variables.end());
	return getTypedValue<T>(it->second);
}


template <typename T, uint16_t id>
void Runner::storevar(T value) {
	std::map<uint16_t, Var>::iterator it = _state->variables.find(id);
	if (it == _state->variables.end()) {
		it = _state->variables.insert(std::make_pair(id, Var(varType<T>(), ""))).first;
	}
	setTypedValue(it->second, value);
}

template <typename T>
void Runner::storevarById(uint16_t id, T value) {
	std::map<uint16_t, Var>::iterator it = _state->variables.find(id);
	if (it == _state->variables.end()) {
		it = _state->variables.insert(std::make_pair(id, Var(varType<T>(), ""))).first;
	}
	setTypedValue(it->second, value);
}

template <typename T>
void Runner::storevarById(uint16_t ctx, uint16_t id, T value) {
	State& ctxstate = _states[ctx].top();
	std::map<uint16_t, Var>::iterator it = ctxstate.variables.find(id);
	if (it == ctxstate.variables.end()) {
		it = ctxstate.variables.insert(std::make_pair(id, Var(varType<T>(), ""))).first;
	}
	setTypedValue(it->second, value);
}

Runner::Runner(CodeImpl* code)
: _code(code)
, _bytecode(0)
, _state(0)
{
	_processors.insert(pr(BC_DPRINT, Printer<double>()));
	_processors.insert(pr(BC_IPRINT, Printer<int64_t>()));
	_processors.insert(pr(BC_SPRINT, Printer<std::string>()));

	_processors.insert(pr(BC_DLOAD, id<double>));
	_processors.insert(pr(BC_ILOAD, id<int64_t>));
	_processors.insert(pr(BC_SLOAD, id<const std::string&>));

	_processors.insert(pr(BC_DLOAD0, retVal<double, 0>));
	_processors.insert(pr(BC_ILOAD0, retVal<int64_t, 0>));
	_processors.insert(pr(BC_SLOAD0, emptyStr));

	_processors.insert(pr(BC_DLOAD1, retVal<double, 1>));
	_processors.insert(pr(BC_ILOAD1, retVal<int64_t, 1>));

	_processors.insert(pr(BC_DLOADM1, retVal<double, -1>));
	_processors.insert(pr(BC_ILOADM1, retVal<int64_t, -1>));

	_processors.insert(pr(BC_DADD, std::plus<double>()));
	_processors.insert(pr(BC_IADD, std::plus<int64_t>()));
	_processors.insert(pr(BC_DSUB, std::minus<double>()));
	_processors.insert(pr(BC_ISUB, std::minus<int64_t>()));
	_processors.insert(pr(BC_DMUL, std::multiplies<double>()));
	_processors.insert(pr(BC_IMUL, std::multiplies<int64_t>()));
	_processors.insert(pr(BC_DDIV, std::divides<double>()));
	_processors.insert(pr(BC_IDIV, std::divides<int64_t>()));
	_processors.insert(pr(BC_DNEG, std::negate<double>()));
	_processors.insert(pr(BC_INEG, std::negate<int64_t>()));
	_processors.insert(pr(BC_IMOD, std::modulus<int64_t>()));

	_processors.insert(pr(BC_I2D, Caster<int64_t, double>()));
	_processors.insert(pr(BC_D2I, Caster<double, int64_t>()));
	_processors.insert(pr(BC_S2I, Caster<std::string, int64_t>()));

	_processors.insert(pr(BC_DCMP, Comparator<double>()));
	_processors.insert(pr(BC_ICMP, Comparator<int64_t>()));

//	_processors.insert(pr(BC_SWAP, ));
//	_processors.insert(pr(BC_POP, ));

//	_processors.insert(pr(BC_LOADDVAR0, std::bind1st(std::mem_fun(&Runner::loadvar<double, 0>), this)));
//	_processors.insert(pr(BC_STOREDVAR0, std::bind1st(std::mem_fun(&Runner::storevar<double, 0>), this)));
}

Runner::~Runner() {
	for (insn2Proc_t::iterator it = _processors.begin(); it != _processors.end(); ++it) {
		delete it->second;
	}
}

Status* Runner::execute(vector<Var*>&) {
	prepareCall(0);

//	_bytecode->dump(std::cerr);
	while (_state->ip < _bytecode->length()) {
		Instruction instruction = getInstruction();

		if (instruction == BC_STOP) {
			break;
		}

		insn2Proc_t::iterator it = _processors.find(instruction);
		if (it != _processors.end()) {
			(*it->second)();
			continue;
		}

		switch (instruction) {
		case BC_LOADDVAR0:
			typedPush<double>(loadvar<double, 0>());
			break;
		case BC_LOADIVAR0:
			typedPush<int64_t>(loadvar<int64_t, 0>());
			break;
		case BC_LOADSVAR0:
			typedPush<std::string>(loadvar<std::string, 0>());
			break;
		case BC_LOADDVAR1:
			typedPush<double>(loadvar<double, 1>());
			break;
		case BC_LOADIVAR1:
			typedPush<int64_t>(loadvar<int64_t, 1>());
			break;
		case BC_LOADSVAR1:
			typedPush<std::string>(loadvar<std::string, 1>());
			break;
		case BC_LOADDVAR2:
			typedPush<double>(loadvar<double, 2>());
			break;
		case BC_LOADIVAR2:
			typedPush<int64_t>(loadvar<int64_t, 2>());
			break;
		case BC_LOADSVAR2:
			typedPush<std::string>(loadvar<std::string, 2>());
			break;
		case BC_LOADDVAR3:
			typedPush<double>(loadvar<double, 3>());
			break;
		case BC_LOADIVAR3:
			typedPush<int64_t>(loadvar<int64_t, 3>());
			break;
		case BC_LOADSVAR3:
			typedPush<std::string>(loadvar<std::string, 3>());
			break;
		case BC_LOADDVAR:
			typedPush<double>(loadvarById<double>(getIdFromStream()));
			break;
		case BC_LOADIVAR:
			typedPush<int64_t>(loadvarById<int64_t>(getIdFromStream()));
			break;
		case BC_LOADSVAR:
			typedPush<std::string>(loadvarById<std::string>(getIdFromStream()));
			break;
		case BC_STOREDVAR0:
			storevar<double, 0>(typedPop<double>());
			break;
		case BC_STOREIVAR0:
			storevar<int64_t, 0>(typedPop<int64_t>());
			break;
		case BC_STORESVAR0:
			storevar<std::string, 0>(typedPop<std::string>());
			break;
		case BC_STOREDVAR1:
			storevar<double, 1>(typedPop<double>());
			break;
		case BC_STOREIVAR1:
			storevar<int64_t, 1>(typedPop<int64_t>());
			break;
		case BC_STORESVAR1:
			storevar<std::string, 1>(typedPop<std::string>());
			break;
		case BC_STOREDVAR2:
			storevar<double, 2>(typedPop<double>());
			break;
		case BC_STOREIVAR2:
			storevar<int64_t, 2>(typedPop<int64_t>());
			break;
		case BC_STORESVAR2:
			storevar<std::string, 2>(typedPop<std::string>());
			break;
		case BC_STOREDVAR3:
			storevar<double, 3>(typedPop<double>());
			break;
		case BC_STOREIVAR3:
			storevar<int64_t, 3>(typedPop<int64_t>());
			break;
		case BC_STORESVAR3:
			storevar<std::string, 3>(typedPop<std::string>());
			break;
		case BC_STOREDVAR:
			storevarById<double>(getIdFromStream(), typedPop<double>());
			break;
		case BC_STOREIVAR:
			storevarById<int64_t>(getIdFromStream(), typedPop<int64_t>());
			break;
		case BC_STORESVAR:
			storevarById<std::string>(getIdFromStream(), typedPop<std::string>());
			break;

		case BC_LOADCTXDVAR: {
			uint16_t ctx = getIdFromStream();
			uint16_t id = getIdFromStream();
			typedPush<double>(loadvarById<double>(ctx, id));
			break;
		}
		case BC_LOADCTXIVAR: {
			uint16_t ctx = getIdFromStream();
			uint16_t id = getIdFromStream();
			typedPush<int64_t>(loadvarById<int64_t>(ctx, id));
			break;
		}
		case BC_LOADCTXSVAR: {
			uint16_t ctx = getIdFromStream();
			uint16_t id = getIdFromStream();
			typedPush<std::string>(loadvarById<std::string>(ctx, id));
			break;
		}

		case BC_STORECTXDVAR: {
			uint16_t ctx = getIdFromStream();
			uint16_t id = getIdFromStream();
			storevarById<double>(ctx, id, typedPop<double>());
			break;
		}
		case BC_STORECTXIVAR: {
			uint16_t ctx = getIdFromStream();
			uint16_t id = getIdFromStream();
			storevarById<int64_t>(ctx, id, typedPop<int64_t>());
			break;
		}
		case BC_STORECTXSVAR: {
			uint16_t ctx = getIdFromStream();
			uint16_t id = getIdFromStream();
			storevarById<std::string>(ctx, id, typedPop<std::string>());
			break;
		}

		case BC_CALL: {
			uint16_t id = getIdFromStream();
			prepareCall(id);
			break;
		}
		case BC_CALLNATIVE:
			break;
		case BC_RETURN:
			returnCall();
			break;
		case BC_JA: {
			int16_t offset = getInt16FromStream() - 2;
			_state->ip += offset;
			break;
		}
		case BC_IFICMPNE: {
			int64_t lower = typedPop<int64_t>();
			int64_t upper = typedPop<int64_t>();
			int64_t offset = getInt16FromStream() - 2;
			if (upper != lower)
				_state->ip += offset;
			break;
		}
		case BC_IFICMPE:{
			int64_t lower = typedPop<int64_t>();
			int64_t upper = typedPop<int64_t>();
			int64_t offset = getInt16FromStream() - 2;
			if (upper == lower)
				_state->ip += offset;
			break;
		}
		case BC_IFICMPG:{
			int64_t lower = typedPop<int64_t>();
			int64_t upper = typedPop<int64_t>();
			int64_t offset = getInt16FromStream() - 2;
			if (upper > lower)
				_state->ip += offset;
			break;
		}
		case BC_IFICMPGE:{
			int64_t lower = typedPop<int64_t>();
			int64_t upper = typedPop<int64_t>();
			int64_t offset = getInt16FromStream() - 2;
			if (upper >= lower)
				_state->ip += offset;
			break;
		}
		case BC_IFICMPL:{
			int64_t lower = typedPop<int64_t>();
			int64_t upper = typedPop<int64_t>();
			int64_t offset = getInt16FromStream() - 2;
			if (upper < lower)
				_state->ip += offset;
			break;
		}
		case BC_IFICMPLE:{
			int64_t lower = typedPop<int64_t>();
			int64_t upper = typedPop<int64_t>();
			int64_t offset = getInt16FromStream() - 2;
			if (upper <= lower)
				_state->ip += offset;
			break;
		}
		case BC_SWAP: {
			Var v1 = _stack.top(); _stack.pop();
			Var v2 = _stack.top(); _stack.pop();
			_stack.push(v1);
			_stack.push(v2);
			break;
		}
		case BC_POP:
			_stack.pop();
			break;
		case BC_DUMP: {
			Var v = _stack.top();
			if (v.type() == VT_DOUBLE)
				std::cout << v.getDoubleValue();
			else if (v.type() == VT_INT)
				std::cout << v.getIntValue();
			else if (v.type() == VT_STRING)
				std::cout << v.getStringValue();
			else
				assert("Unknown type for DUMP" == 0);
			break;
		}
		case BC_BREAK:; break;
		default:
			std::cerr << instruction << " instruction is not supported" << std::endl;
			assert("instruction is not supported" == 0);
		}
	}

	return 0;
}

void Runner::prepareCall(uint16_t functionId) {
	_states[functionId].push(State());
	_states[functionId].top().ip = 0;

	_state = &_states[functionId].top();

	_callStack.push(functionId);

	BytecodeFunction* fun = (BytecodeFunction*) _code->functionById(functionId);
	_bytecode = fun->bytecode();
}

void Runner::returnCall() {
	const uint16_t functionId = _callStack.top();
	_states[functionId].pop();

	_callStack.pop();

	const uint16_t parentFunctionId = _callStack.top();
	_state = &_states[parentFunctionId].top();

	BytecodeFunction* fun = (BytecodeFunction*) _code->functionById(parentFunctionId);
	_bytecode = fun->bytecode();
}

// Helpers:
namespace detail {
	template<typename T>
	T typedPop(Code* code, std::stack<Var>& stack) {
		T val = getTypedValue<T>(stack.top());
		stack.pop();
		return val;
	}

	template<typename T>
	void typedPush(Code* code, std::stack<Var>& stack, T val) {
		Var var(varType<T>(), "");
		setTypedValue(var, val);
		stack.push(var);
	}

	template<>
	std::string typedPop<std::string>(Code* code, std::stack<Var>& stack) {
		uint16_t id = typedPop<int64_t>(code, stack);
		return code->constantById(id);
	}

	 template<>
	 void typedPush<std::string>(Code* code, std::stack<Var>& stack, std::string val) {
		typedPush<int64_t>(code, stack, code->makeStringConstant(val));
	}

	template<>
	void typedPop<void>(Code* code, std::stack<Var>& stack) {}
}

template<typename T>
T Runner::typedPop() {
	return detail::typedPop<T>(_code, _stack);
}

template<typename T>
void Runner::typedPush(T val) {
	detail::typedPush(_code, _stack, val);
}

Bytecode* Runner::bytecode() {
	return _bytecode;
}

uint16_t Runner::getIdFromStream() {
	uint16_t val = bytecode()->getInt16(_state->ip);
	_state->ip += 2; //todo move this logic to another location
	return val;
}

int16_t Runner::getInt16FromStream() {
	uint16_t val = bytecode()->getInt16(_state->ip);
	_state->ip += 2; //todo move this logic to another location
	return val;
}

double Runner::getDoubleFromStream() {
	double val = bytecode()->getDouble(_state->ip);
	_state->ip += 8;
	return val;
}

int64_t Runner::getIntegerFromStream() {
	int64_t val = bytecode()->getInt64(_state->ip);
	_state->ip += 8;
	return val;
}

const std::string& Runner::getStringFromStream() {
	uint16_t id = getIdFromStream();
	return _code->constantById(id);
}

Instruction Runner::getInstruction() {
	return _bytecode->getInsn(_state->ip++)	;
}

}
