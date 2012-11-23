#include "Runner.h"

namespace mathvm {

namespace {

template <typename T>
void print(T val) {
	std::cout << val << std::endl;
}

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

}

Runner::Runner(CodeImpl* code)
: _code(code)
, _bytecode(0)
, _ip(0)
{
	_processors.insert(pr(BC_DPRINT, print<double>));
	_processors.insert(pr(BC_IPRINT, print<int64_t>));
	_processors.insert(pr(BC_SPRINT, print<const std::string&>));

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
}

Runner::~Runner() {
	for (insn2Proc_t::iterator it = _processors.begin(); it != _processors.end(); ++it) {
		delete it->second;
	}
}

Status* Runner::execute(vector<Var*>&) {
	BytecodeFunction* fun = (BytecodeFunction*) _code->functionById(0);
	_bytecode = fun->bytecode();

	std::stack<Var> stack;
	_ip = 0;
	while (_ip < _bytecode->length()) {
		Instruction instruction = getInstruction();

		insn2Proc_t::iterator it = _processors.find(instruction);
		if (it != _processors.end()) {
			(*it->second)();
			continue;
		}

		switch (instruction) {
			default: ; 
		}
	}

	return 0;
}

// Helpers:
int64_t Runner::popInteger() {
	int64_t val = _stack.top().getIntValue();
	_stack.pop();
	return val;
}

void Runner::pushInteger(int64_t val) {
	Var var(VT_INT, "");
	var.setIntValue(val);
	_stack.push(var);

}

double Runner::popDouble() {
	double val = _stack.top().getDoubleValue();
	_stack.pop();
	return val;
}

void Runner::pushDouble(double val) {
	Var var(VT_DOUBLE, "");
	var.setDoubleValue(val);
	_stack.push(var);
}

const std::string Runner::popString() {
	uint16_t id = popInteger();
	return _code->constantById(id);
}

void Runner::pushString(const std::string& val) {
	Var var(VT_INT, "");
	var.setIntValue(_code->makeStringConstant(val));
	_stack.push(var);
}

Bytecode* Runner::bytecode() {
	return _bytecode;
}

uint16_t Runner::getIdFromStream() {
	uint16_t val = bytecode()->getInt16(_ip);
	_ip += 2; 
	return val;
}

double Runner::getDoubleFromStream() {
	double val = bytecode()->getDouble(_ip);
	_ip += 8;
	return val;
}

int64_t Runner::getIntegerFromStream() {
	int64_t val = bytecode()->getInt64(_ip);
	_ip += 8;
	return val;
}

const std::string& Runner::getStringFromStream() {
	uint16_t id = getIdFromStream();
	return _code->constantById(id);
}

Instruction Runner::getInstruction() {
	return _bytecode->getInsn(_ip++)	;
}

}