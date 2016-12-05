//
// Created by svloyso on 26.11.16.
//

#ifndef MATHVM_STACK_H
#define MATHVM_STACK_H

#include <stack>
#include <mathvm.h>

namespace mathvm {

struct Variable {
	char type;
	union {
		double d;
		int64_t i;
		uint16_t s;
	} value;

	Variable(int64_t i) : type('i') {
		value.i = i;
	}

	Variable(double d) : type('d') {
		value.d = d;
	}

	Variable(uint16_t s) : type('s') {
		value.s = s;
	}
};

class Stack {
	std::stack<Variable> stack;

public:
	void ipush(int64_t val) {
		stack.push(Variable(val));
	}

	void dpush(double val) {
		stack.push(Variable(val));
	}

	void spush(uint16_t val) {
		stack.push(Variable(val));
	}

	int64_t itop() {
		return stack.top().value.i;
	}

	double dtop() {
		return stack.top().value.d;
	}

	uint16_t stop() {
		return stack.top().value.s;
	}

	void pop() {
		stack.pop();
	}

	void iadd() {
		if (stack.size() < 2) throw Status::Error("Can not pop from empty stack");
		Variable e = stack.top();
		stack.pop();
		if(e.type != 'i' || stack.top().type != 'i') throw Status::Error("Invalid operand types");
		stack.top().value.i += e.value.i;
	}

	void dadd() {
		if (stack.size() < 2) throw Status::Error("Can not pop from empty stack");
		Variable e = stack.top();
		stack.pop();
		if(e.type != 'd' || stack.top().type != 'd') throw Status::Error("Invalid operand types");
		stack.top().value.d += e.value.d;
	}

	void isub() {
		if (stack.size() < 2) throw Status::Error("Can not pop from empty stack");
		Variable e = stack.top();
		stack.pop();
		if(e.type != 'i' || stack.top().type != 'i') throw Status::Error("Invalid operand types");
		stack.top().value.i -= e.value.i;
	}

	void dsub() {
		if (stack.size() < 2) throw Status::Error("Can not pop from empty stack");
		Variable e = stack.top();
		stack.pop();
		if(e.type != 'd' || stack.top().type != 'd') throw Status::Error("Invalid operand types");
		stack.top().value.d -= e.value.d;
	}

	void imul() {
		if (stack.size() < 2) throw Status::Error("Can not pop from empty stack");
		Variable e = stack.top();
		stack.pop();
		if(e.type != 'i' || stack.top().type != 'i') throw Status::Error("Invalid operand types");
		stack.top().value.i *= e.value.i;
	}

	void dmul() {
		if (stack.size() < 2) throw Status::Error("Can not pop from empty stack");
		Variable e = stack.top();
		stack.pop();
		if(e.type != 'd' || stack.top().type != 'd') throw Status::Error("Invalid operand types");
		stack.top().value.d *= e.value.d;
	}

	void ddiv() {
		if (stack.size() < 2) throw Status::Error("Can not pop from empty stack");
		Variable e = stack.top();
		stack.pop();
		if(e.type != 'd' || stack.top().type != 'd') throw Status::Error("Invalid operand types");
		stack.top().value.d /= e.value.d;
	}

	void idiv() {
		if (stack.size() < 2) throw Status::Error("Can not pop from empty stack");
		Variable e = stack.top();
		stack.pop();
		if(e.type != 'i' || stack.top().type != 'i') throw Status::Error("Invalid operand types");
		stack.top().value.i /= e.value.i;
	}

	void imod() {
		if (stack.size() < 2) throw Status::Error("Can not pop from empty stack");
		Variable e = stack.top();
		stack.pop();
		if(e.type != 'i' || stack.top().type != 'i') throw Status::Error("Invalid operand types");
		stack.top().value.i %= e.value.i;
	}

	void ineg() {
		if(stack.size() < 1) throw Status::Error("Can not pop from empty stack");
		if(stack.top().type != 'i') throw Status::Error("Invalid operand types");
		stack.top().value.i *= -1;
	}

	void dneg() {
		if(stack.size() < 1) throw Status::Error("Can not pop from empty stack");
		if(stack.top().type != 'd') throw Status::Error("Invalid operand types");
		stack.top().value.d *= -1;
	}

	void iand() {
		if (stack.size() < 2) throw Status::Error("Can not pop from empty stack");
		Variable e = stack.top();
		stack.pop();
		if(e.type != 'i' || stack.top().type != 'i') throw Status::Error("Invalid operand types");
		stack.top().value.i &= e.value.i;
	}

	void ior() {
		if (stack.size() < 2) throw Status::Error("Can not pop from empty stack");
		Variable e = stack.top();
		stack.pop();
		if(e.type != 'i' || stack.top().type != 'i') throw Status::Error("Invalid operand types");
		stack.top().value.i |= e.value.i;
	}

	void ixor() {
		if (stack.size() < 2) throw Status::Error("Can not pop from empty stack");
		Variable e = stack.top();
		stack.pop();
		if(e.type != 'i' || stack.top().type != 'i') throw Status::Error("Invalid operand types");
		stack.top().value.i ^= e.value.i;
	}

	void i2d() {
		if(stack.size() < 1) throw Status::Error("Can not pop from empty stack");
		if(stack.top().type != 'i') throw Status::Error("Invalid operand types");
		stack.top().type = 'd';
		stack.top().value.d = stack.top().value.i;
	}

	void d2i() {
		if(stack.size() < 1) throw Status::Error("Can not pop from empty stack");
		if(stack.top().type != 'd') throw Status::Error("Invalid operand types");
		stack.top().type = 'i';
		stack.top().value.i = (int64_t)stack.top().value.d;
	}

	void swap() {
		Variable e1 = stack.top();
		stack.pop();
		Variable e2 = stack.top();
		stack.pop();
		stack.push(e1);
		stack.push(e2);
	}

	void cmp() {
		Variable e2 = stack.top();
		stack.pop();
		Variable e1 = stack.top();
		stack.pop();
		if(e1.type != e2.type || e1.type == 's') {
			throw Status::Error((std::string("Invalid types for compare: ") + e1.type + " " + e1.type).c_str());
		}
		Variable res((int64_t)0);
		if(e1.type == 'i') {
			if(e1.value.i > e2.value.i) res.value.i = 1;
			else if(e1.value.i < e2.value.i) res.value.i = -1;
		}
		if(e1.type == 'd') {
			if(e1.value.d > e2.value.d) res.value.i = 1;
			else if(e1.value.d < e2.value.d) res.value.i = -1;
		}
		stack.push(res);
	}

	char ttop() {
		return stack.top().type;
	}

	bool empty() {
		return stack.empty();
	}
};

}

#endif //MATHVM_STACK_H
