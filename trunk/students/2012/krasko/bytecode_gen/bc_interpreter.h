/*
 * bc_interpreter.h
 *
 *  Created on: 18.11.2012
 *      Author: Evgeniy Krasko
 */

#ifndef BC_INTERPRETER_H_
#define BC_INTERPRETER_H_

#include <vector>
#include <iostream>
#include "context.h"
using namespace std;
using namespace mathvm;

class BC_Interpreter: public Code {
	int lengths[256];
	void fillLengths() {
#define INSERT_LEN(b, d, l) lengths[(BC_##b)] = l;
		FOR_BYTECODES(INSERT_LEN)
#undef INSERT_LEN
	}

	vector<stacktype> stack;

	void push(stacktype s) {
		stack.push_back(s);
	}

	stacktype pop() {
		stacktype res = stack[stack.size() - 1];
		stack.pop_back();
		return res;
	}

	BytecodeFunction *funcs;
public:
	Status* execute(vector<Var*> & vars) {
		fillLengths();
		Context context;
		Code::FunctionIterator iter2(this);
		while (iter2.hasNext()) {
			context.setFunction((BytecodeFunction*) iter2.next());
		}
		call(0, &context);
		return 0;
	}

	void call(int id, Context * context) {
		BytecodeFunction * f = context->enter(id);
		Bytecode * bc = f->bytecode();
		double dbl;
		int64_t i64;
		int16_t i16;
		stacktype t1, t2;
		Instruction insn;
		for (unsigned i = 0; i < bc->length();) {
			insn = bc->getInsn(i);
			switch (insn) {
			case BC_DLOAD:
				push(bc->getDouble(i + 1));
				break;
			case BC_ILOAD:
				push(bc->getInt64(i + 1));
				break;
			case BC_SLOAD:
				push(bc->getInt16(i + 1));
				break;
			case BC_DLOAD0:
				push(0.0);
				break;
			case BC_ILOAD0:
				push((int64_t) 0);
				break;
			case BC_SLOAD0:
				push((int16_t) 0);
				break;
			case BC_DLOAD1:
				push(1.0);
				break;
			case BC_ILOAD1:
				push((int64_t) 1);
				break;
			case BC_DLOADM1:
				push(-1.0);
				break;
			case BC_ILOADM1:
				push((int64_t) -1);
				break;
#define IOP(x)  i64 = pop();					\
				push(i64 x (int64_t) pop());	\
				break;
#define DOP(x)  dbl = pop();					\
				push(dbl x (double) pop());		\
				break;
			case BC_DADD:
				DOP(+)
			case BC_IADD:
				IOP(+)
			case BC_DSUB:
				DOP(-)
			case BC_ISUB:
				IOP(-)
			case BC_DMUL:
				DOP(*)
			case BC_IMUL:
				IOP(*)
			case BC_DDIV:
				DOP(/)
			case BC_IDIV:
				IOP(/)
			case BC_IMOD:
				IOP(%)
#undef IOP(x)
#undef DOP(x)
			case BC_DNEG:
				push(-(double) pop());
				break;
			case BC_INEG:
				push(-(int64_t) pop());
				break;
			case BC_DPRINT:
				cout << (double) pop();
				break;
			case BC_IPRINT:
				cout << (int64_t) pop();
				break;
			case BC_SPRINT:
				cout << this->constantById((int16_t) pop());
				break;
			case BC_D2I:
				dbl = pop();
				i64 = dbl;
				push(i64);
				break;
			case BC_I2D:
				i64 = pop();
				dbl = i64;
				push(dbl);
				break;
			case BC_S2I:
				//TODO
				break;
			case BC_SWAP:
				t1 = pop();
				t2 = pop();
				stack.push_back(t1);
				stack.push_back(t2);
				break;
			case BC_POP:
				i64 = pop();
				break;
#define LD_STOR(x, v)   case BC_LOADCTX##x##VAR:										\
				stack.push_back(context->get(bc->getInt16(i + 1), bc->getInt16(i + 3)));\
				break;																	\
			case BC_STORECTX##x##VAR:													\
				v   = pop();															\
				context->set(bc->getInt16(i + 1),bc->getInt16(i + 3), v);				\
				break;
			LD_STOR(I, i64)
			LD_STOR(D, dbl)
			LD_STOR(S, i16)
#undef LD_STOR(x, v)
			case BC_CALL:
				call(bc->getInt16(i + 1), context);
				break;
			case BC_RETURN:
				context->leave(id);
				return;
			case BC_JA:
				i += bc->getInt16(i + 1) + 1;
				continue;
#define CONDJUMP(x) i64 = pop();				\
				if (i64 x (int64_t) pop()) {	\
					i += bc->getInt16(i + 1) + 1;\
					continue;					\
				}								\
				break;
			case BC_IFICMPNE:
				CONDJUMP(!=)
			case BC_IFICMPE:
				CONDJUMP(==)
			case BC_IFICMPGE:
				CONDJUMP(>=)
			case BC_IFICMPG:
				CONDJUMP(>)
			case BC_IFICMPLE:
				CONDJUMP(<=)
			case BC_IFICMPL:
				CONDJUMP(<)
#undef CONDJUMP(x)
			default:
				break;
			}
			i += lengths[insn];
		}
		context->leave(id);
	}
};
#endif /* BC_INTERPRETER_H_ */
