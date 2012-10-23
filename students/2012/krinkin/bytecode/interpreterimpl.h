#ifndef __INTERPRETER_IMPL_H__
#define __INTERPRETER_IMPL_H__

#include "mathvm.h"

#include <vector>
#include <string>
#include <stack>

using namespace mathvm;

class InterpreterImpl
{
private:
	union stack_value
	{
		int64_t ival;
		double dval;
		char const *sval;
		
		stack_value(int64_t val) : ival(val) {};
		stack_value(double val) : dval(val) {};
		stack_value(char const *val) : sval(val) {};
		
		operator int64_t() { return ival; }
		operator double() { return dval; }
		operator char const *() { return sval; }
	};

	Code *m_code;
	std::stack<stack_value> m_stack;
	std::vector<char *> m_locals;
	
	std::vector<int64_t> m_iregisters;
	std::vector<double> m_dregisters;
	std::vector<char const *> m_sregisters;
	
	std::string empty;
	
	void call(BytecodeFunction *bytecode);
	
	void store_value_at(int64_t value, uint16_t scope, uint16_t offset);
	void store_value_at(double value, uint16_t scope, uint16_t offset);
	void store_value_at(char const *value, uint16_t scope, uint16_t offset);
	
	void load_value_from(int64_t &value, uint16_t scope, uint16_t offset);
	void load_value_from(double &value, uint16_t scope, uint16_t offset);
	void load_value_from(char const * &value, uint16_t scope, uint16_t offset);
	
	bool jump(Instruction insn) const;
	bool one_argument(Instruction insn) const;
	bool two_arguments(Instruction insn) const;
	
	size_t count_functions() const;
	void clear();

public:
	InterpreterImpl(Code *code) : m_code(code) {}
	virtual ~InterpreterImpl() {}
	
	void run();
};

#endif /* __INTERPRETER_IMPL_H__ */
