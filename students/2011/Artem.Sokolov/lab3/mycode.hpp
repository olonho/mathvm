#ifndef _MYCODE_HPP_
#define _MYCODE_HPP_

#include <cstdlib>
#include "mathvm.h"
#include <ostream>

union Variable {
	double d;
	int64_t i;
	char const *s;
};

union StackFrameUnit {
	Variable v;
	StackFrameUnit *previous;
	uint16_t context_id;
};

class MyCode: public mathvm::Code {
	StackFrameUnit *current_frame;
	StackFrameUnit *call_stack;

	Variable *stack;
	Variable *top;

	StackFrameUnit *frame_top;
	StackFrameUnit *frame;

	std::ostream &output_stream;

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

	void push_string(char const *string) {
		top->s = string;
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

	StackFrameUnit *get_frame(uint16_t context_id) {
		StackFrameUnit *frame = current_frame;
		assert(context_id <= frame[-2].context_id);
		while (frame[-2].context_id != context_id)
			frame = frame[-1].previous;
		return frame;
	}

	void set_frame(uint16_t context_id, uint16_t id) {
		if (current_frame[-2].context_id == context_id) {
			frame_top += id + 1;
			frame = current_frame;
		} else if (context_id == 0) {
			frame = call_stack + 2;
		} else {
			frame = get_frame(context_id);
		}
	}

	mathvm::Status *execute_bytecode(mathvm::Bytecode *bytecode) {
		mathvm::Instruction instruction;
		uint32_t index = 0;

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

		    case mathvm::BC_STORECTXIVAR: {
		    	uint16_t context_id = bytecode->getInt16(index);
		    	index += 2;
		    	uint16_t id = bytecode->getInt16(index);
		    	index += 2;

		    	set_frame(context_id, id);

		    	frame[id].v.i = pop_int();
		    }   break;
		    case mathvm::BC_STORECTXDVAR: {
		    	uint16_t context_id = bytecode->getInt16(index);
				index += 2;
				uint16_t id = bytecode->getInt16(index);
				index += 2;

				set_frame(context_id, id);

				frame[id].v.d = pop_double();
		    }   break;
		    case mathvm::BC_STORECTXSVAR: {
				uint16_t context_id = bytecode->getInt16(index);
				index += 2;
				uint16_t id = bytecode->getInt16(index);
				index += 2;

				set_frame(context_id, id);

				frame[id].v.s = pop_string();
			}   break;
		    case mathvm::BC_LOADCTXSVAR: {
				uint16_t context_id = bytecode->getInt16(index);
				index += 2;
				uint16_t id = bytecode->getInt16(index);
				index += 2;
				StackFrameUnit *frame = get_frame(context_id);
				push_string(frame[id].v.s);
			}   break;
		    case mathvm::BC_LOADCTXDVAR: {
				uint16_t context_id = bytecode->getInt16(index);
				index += 2;
				uint16_t id = bytecode->getInt16(index);
				index += 2;
				StackFrameUnit *frame = get_frame(context_id);
				push_double(frame[id].v.d);
			}   break;
		    case mathvm::BC_LOADCTXIVAR: {
				uint16_t context_id = bytecode->getInt16(index);
				index += 2;
				uint16_t id = bytecode->getInt16(index);
				index += 2;
				StackFrameUnit *frame = get_frame(context_id);
				int lol = frame[id].v.i;
				push_int(lol);
			}   break;

		    case mathvm::BC_IADD:
				push_int(pop_int() + pop_int());
				break;
		    case mathvm::BC_DADD:
				push_double(pop_double() + pop_double());
				break;
		    case mathvm::BC_IMUL:
				push_int(pop_int() * pop_int());
				break;
		    case mathvm::BC_DMUL:
		    	push_double(pop_double() * pop_double());
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
				push_double(-pop_double());
				break;

		    case mathvm::BC_I2D:
				top->d = (double)top->i;
				break;
		    case mathvm::BC_D2I:
				top->i = (int)top->d;
				break;

		    case mathvm::BC_IPRINT:
				output_stream << pop_int();
				break;
		    case mathvm::BC_DPRINT:
		    	output_stream << pop_double();
				break;
		    case mathvm::BC_SPRINT:
		    	output_stream << pop_string();
				break;

		    case mathvm::BC_CALL: {
		    	uint16_t function_id = bytecode->getInt16(index);
				index += 2;

				frame_top->context_id = function_id;
				frame_top[1].previous = current_frame;
				frame_top += 2;
				current_frame = frame_top;

		    	mathvm::BytecodeFunction *function = (mathvm::BytecodeFunction *) functionById(function_id);
				execute_bytecode(function->bytecode());
		    }	break;
			case mathvm::BC_RETURN: {
				frame_top = current_frame - 2;
				current_frame = current_frame[-1].previous;
				return NULL;
			}	break;

		    case mathvm::BC_JA:
				index += bytecode->getInt16(index);
				break;
		    case mathvm::BC_IFICMPNE: {
		    	uint16_t offset = bytecode->getInt16(index);
				if (pop_int() != pop_int())
					index += offset;
				else
					index += 2;
		    }	break;
		    case mathvm::BC_IFICMPE: {
		    	uint16_t offset = bytecode->getInt16(index);
				if (pop_int() == pop_int())
					index += offset;
				else
					index += 2;
		    }	break;
		    case mathvm::BC_IFICMPG: {
		    	uint16_t offset = bytecode->getInt16(index);
				if (pop_int() > pop_int())
					index += offset;
				else
					index += 2;
		    }	break;
		    case mathvm::BC_IFICMPGE: {
		    	uint16_t offset = bytecode->getInt16(index);
				if (pop_int() >= pop_int())
					index += offset;
				else
					index += 2;
		    }	break;
		    case mathvm::BC_IFICMPL: {
		    	uint16_t offset = bytecode->getInt16(index);
				if (pop_int() < pop_int())
					index += offset;
				else
					index += 2;
		    }	break;
		    case mathvm::BC_IFICMPLE: {
		    	uint16_t offset = bytecode->getInt16(index);
				if (pop_int() <= pop_int())
					index += offset;
				else
					index += 2;
		    }	break;
		    case mathvm::BC_STOP:
		    	return NULL;
		    	break;
		    default:
				assert(false);
				break;
		    }
		}

		return NULL;
	}
public:
	MyCode(std::ostream &output_stream): output_stream(output_stream) {
		call_stack = (StackFrameUnit *)std::malloc(sizeof(StackFrameUnit) * 8192);
		current_frame = call_stack + 2;
		frame_top = current_frame;

		stack = (Variable *)std::malloc(sizeof(Variable) * 1024);
		top = stack;
	}

	~MyCode() {
		free(call_stack);
		free(stack);
	}

	mathvm::Status *execute(std::vector<mathvm::Var*> &vars) {
		mathvm::BytecodeFunction *top = (mathvm::BytecodeFunction *) functionById(0);
		current_frame[-1].previous = NULL;
		current_frame[-2].context_id = 0;;
		return execute_bytecode(top->bytecode());
	}
};

#endif // _MYCODE_HPP_
