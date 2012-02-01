#ifndef _MYCODE_HPP_
#define _MYCODE_HPP_

#include <cstdlib>
#include "mathvm.h"

union Variable {
	double d;
	int64_t i;
	char const *s;
};

union StackFrameUnit {
	Variable v;
	StackFrameUnit *previous;
};

class MyCode: public mathvm::Code {
	StackFrameUnit *current_unit;
	StackFrameUnit *call_stack;

	Variable *stack;
	Variable *top;

	uint16_t current_context;

	void push_int(int64_t value) {
		top->i = value;
		++top;
	}

	void push_double(double value) {
		top->d = value;
		++top;
	}

	void push_string(uint16_t id) {
		top->s = constantById(id).c_str();
		++top;
	}

	int64_t pop_int() {
		--top;
		return top->i;
	}

	double pop_double() {
		--top;
		return top->d;
	}

	char const *pop_string() {
		--top;
		return top->s;
	}

	mathvm::Status *execute_bytecode(mathvm::Bytecode *bytecode) {
		mathvm::Instruction instruction;
		uint32_t index;

		while (true) {
		    instruction = bytecode->getInsn(index++);

		    switch (instruction) {
		    case mathvm::BC_ILOAD:
		     	push_int(bytecode->getInt64(index));
		     	index += 8;
		     	break;
		    case mathvm::BC_ILOAD0:
		    	push_int((int64_t)0);
		    	break;
		    case mathvm::BC_ILOAD1:
		    	push_int((int64_t)1);
		    	break;
		    case mathvm::BC_ILOADM1:
		    	push_int((int64_t)-1);
		    	break;
		    case mathvm::BC_DLOAD:
		    	push_double(bytecode->getDouble(index));
		    	index += 8;
				break;
		    case mathvm::BC_DLOAD0:
		    	push_double((double)0);
		    	break;
		    case mathvm::BC_DLOAD1:
		    	push_double((double)1);
		    	break;
		    case mathvm::BC_DLOADM1:
		    	push_double((double)-1);
		    	break;
		    case mathvm::BC_SLOAD:
		    	push_string(bytecode->getInt16(index));
		    	index += 2;
		    	break;
		    case mathvm::BC_IADD:
				push_int(pop_int() + pop_int());
				break;
		    case mathvm::BC_DADD:
				push_int(pop_double() + pop_double());
				break;
		    case mathvm::BC_IMUL:
				push_int(pop_int() * pop_int());
				break;
		    case mathvm::BC_DMUL:
				push_int(pop_double() * pop_double());
				break;
		    case mathvm::BC_ISUB:
		        push_int(pop_int() - pop_int());
		        break;
		    case mathvm::BC_DSUB:
		        push_double(pop_double() - pop_double());
		        break;
		    case mathvm::BC_IDIV:
		        push_int(pop_int() / pop_int());
		        break;
		    case mathvm::BC_DDIV:
		        push_double(pop_double() / pop_double());
		        break;
		    case mathvm::BC_INEG:
				push_int(-pop_int());
				break;
		    case mathvm::BC_DNEG:
				push_int(-pop_double());
				break;
		    case mathvm::BC_I2D:
				top->d = (double)top->i;
				break;
		    case mathvm::BC_D2I:
				top->i = (int)top->d;
				break;
		    case mathvm::BC_IPRINT:
				std::cout << pop_int();
				break;
		    case mathvm::BC_DPRINT:
		    	std::cout << pop_double();
				break;
		    case mathvm::BC_SPRINT:
		    	std::cout << pop_string();
				break;
		    case mathvm::BC_JA:
				index += bytecode->getInt16(index);
				break;
		    case mathvm::BC_IFICMPNE:
				if (pop_int() != pop_int())
					index += bytecode->getInt16(index);
				break;
		    case mathvm::BC_IFICMPE:
		    	if (pop_int() == pop_int())
					index += bytecode->getInt16(index);
				break;
		    case mathvm::BC_IFICMPG:
		    	if (pop_int() > pop_int())
		    		index += bytecode->getInt16(index);
		    	break;
		    case mathvm::BC_IFICMPGE:
		    	if (pop_int() >= pop_int())
					index += bytecode->getInt16(index);
				break;
		    case mathvm::BC_IFICMPL:
		    	if (pop_int() < pop_int())
					index += bytecode->getInt16(index);
				break;
		    case mathvm::BC_IFICMPLE:
		    	if (pop_int() <= pop_int())
					index += bytecode->getInt16(index);
				break;
		    case mathvm::BC_STOP:
		    	return NULL;
		    default:
				assert(false);
				break;
		    }
		}

		return NULL;
	}
public:
	MyCode() {
		call_stack = (StackFrameUnit *)std::malloc(sizeof(StackFrameUnit) * 8192);
		current_unit = call_stack;

		stack = (Variable *)std::malloc(sizeof(Variable) * 1024);
		top = stack;

		current_context = 0;
	}

	~MyCode() {
		free(call_stack);
		free(stack);
	}

	mathvm::Status *execute(std::vector<mathvm::Var*> &vars) {
		mathvm::BytecodeFunction *top = (mathvm::BytecodeFunction *) functionById(0);
		return execute_bytecode(top->bytecode());
	}
};

#endif // _MYCODE_HPP_
