/*
 * BytecodeImpl.cpp
 *
 *  Created on: 24.10.2012
 *      Author: temp1ar
 */

#include "BytecodeImpl.h"
#include <stack>

namespace mathvm {

BytecodeImpl::BytecodeImpl() {
}

BytecodeImpl::~BytecodeImpl() {
}

Status* BytecodeImpl::execute(vector<Var*>& vars) {
	BytecodeFunction* func = (BytecodeFunction*) functionByName("<top>");
	executeFunction(func);
	return 0;
}

union value {
	int64_t i;
	double d;
};

static size_t bclen(Instruction insn) {
	static const struct
	{
			const char* name;
			Instruction insn;
			size_t length;
	} names[] = {
			#define BC_NAME(b, d, l) {#b, BC_##b, l},
			FOR_BYTECODES(BC_NAME)
	};

	if (insn >= BC_INVALID && insn < BC_LAST) return names[insn].length;

	assert(false);
	return 0;
}

void BytecodeImpl::executeFunction(BytecodeFunction* f) {
	Bytecode* code = f->bytecode();
	std::stack<value> stack;
	std::vector<uint64_t> storedInts(4);
	std::map<uint16_t, uint64_t> storedIntsCustom;

	for(uint32_t index = 0; index < code->length();) {
		Instruction insn = code->getInsn(index);
		//std::cout << "read: " << insn << std::endl;
		value v1;
		value v2;
		value v;
		value res;
		switch(insn) {
			case BC_INVALID:
				std::cerr << "INVALID INSTRUCTION" << std::endl;
				return;
				break;
			case BC_ILOAD:
				v.i = code->getInt64(index + 1);
				std::cerr << "BC_ILOAD " << v.i << std::endl;
				stack.push(v);
				break;
			case BC_ILOAD0:
				v.i = 0;
				std::cerr << "BC_ILOAD0 " << std::endl;
				stack.push(v);
				break;
			case BC_ILOAD1:
				v.i = 1;
				std::cerr << "BC_ILOAD1 " << std::endl;
				stack.push(v);
				break;
			case BC_ILOADM1:
				v.i = -1;
				std::cerr << "BC_ILOADM1 " << std::endl;
				stack.push(v);
				break;
			case BC_IADD:
				v1 = stack.top();
				stack.pop();
				v2 = stack.top();
				stack.pop();
				std::cerr << "BC_IADD " << v1.i << " " << v2.i << std::endl;
				res.i = v1.i + v2.i;
				stack.push(res);
				break;
			case BC_ISUB:
				v1 = stack.top();
				stack.pop();
				v2 = stack.top();
				stack.pop();
				res.i = v1.i - v2.i;
				std::cerr << "BC_ISUB " << v1.i << " " << v2.i << std::endl;
				stack.push(res);
				break;
			case BC_IMUL:
				v1 = stack.top();
				stack.pop();
				v2 = stack.top();
				stack.pop();
				std::cerr << "BC_IMUL " << v1.i << " " << v2.i << std::endl;
				res.i = v1.i * v2.i;
				stack.push(res);
				break;
			case BC_IDIV:
				v1 = stack.top();
				stack.pop();
				v2 = stack.top();
				stack.pop();
				std::cerr << "BC_IDIV " << v1.i << " " << v2.i << std::endl;
				res.i = v1.i / v2.i;
				stack.push(res);
				break;
			case BC_IMOD:
				v1 = stack.top();
				stack.pop();
				v2 = stack.top();
				stack.pop();
				std::cerr << "BC_IMOD " << v1.i << " " << v2.i << std::endl;
				res.i = v1.i % v2.i;
				stack.push(res);
				break;
			case BC_INEG:
				v1 = stack.top();
				stack.pop();
				res.i = -v1.i;
				std::cerr << "BC_INEG " << std::endl;
				stack.push(res);
				break;
			case BC_IPRINT:
				v1 = stack.top();
				stack.pop();
				std::cerr << "BC_IPRINT " << std::endl;
				std::cout << v1.i;
				break;
			case BC_SWAP:
				v1 = stack.top();
				stack.pop();
				v2 = stack.top();
				stack.pop();
				stack.push(v1);
				stack.push(v2);
				std::cerr << "BC_SWAP" << std::endl;
				break;
			case BC_POP:
				stack.pop();
				std::cerr << "BC_POP" << std::endl;
				break;
			case BC_LOADIVAR0:
				v.i = storedInts.at(0);
				stack.push(v);
				std::cerr << "BC_LOADIVAR0" << std::endl;
				break;
			case BC_LOADIVAR1:
				v.i = storedInts.at(1);
				stack.push(v);
				std::cerr << "BC_LOADIVAR1" << std::endl;
				break;
			case BC_LOADIVAR2:
				v.i = storedInts.at(2);
				stack.push(v);
				std::cerr << "BC_LOADIVAR2" << std::endl;
				break;
			case BC_LOADIVAR3:
				v.i = storedInts.at(3);
				stack.push(v);
				std::cerr << "BC_LOADIVAR3" << std::endl;
				break;
			case BC_STOREIVAR0:
				v = stack.top();
				storedInts[0] = v.i;
				stack.pop();
				std::cerr << "BC_STOREIVAR0" << std::endl;
				break;
			case BC_STOREIVAR1:
				v = stack.top();
				storedInts[1] = v.i;
				stack.pop();
				std::cerr << "BC_STOREIVAR1" << std::endl;
				break;
			case BC_STOREIVAR2:
				v = stack.top();
				storedInts[2] = v.i;
				stack.pop();
				std::cerr << "BC_STOREIVAR2" << std::endl;
				break;
			case BC_STOREIVAR3:
				v = stack.top();
				storedInts[3] = v.i;
				stack.pop();
				std::cerr << "BC_STOREIVAR3" << std::endl;
				break;
			case BC_LOADIVAR:
				v.i = storedIntsCustom[code->getUInt16(index + 1)];
				stack.push(v);
				std::cerr << "BC_LOADIVAR @" << code->getUInt16(index + 1) << " = " << v.i << std::endl;
				break;
			case BC_STOREIVAR:
				v = stack.top();
				stack.pop();
				storedIntsCustom[code->getUInt16(index + 1)] = v.i;
				std::cerr << "BC_STOREIVAR @" << code->getUInt16(index + 1) << " = " << v.i << std::endl;
				break;
			case BC_ICMP:
				v1 = stack.top();
				stack.pop();
				v2 = stack.top();
				stack.push(v1);
				res.i = compare(v1.i, v2.i);
				stack.push(res);
				std::cerr << "BC_ICMP " << v1.i << " " << v2.i << " " << res.i << std::endl;
				break;
			case BC_DUMP:
				v1 = stack.top();
				std::cout << "dump TOS: " << v1.i << " " << v1.d << std::endl;
				break;
			case BC_STOP:
				std::cout << "Stopping machine" << std::endl;
				break;
			case BC_JA:
				index += code->getUInt16(index + 1) + 1;
				std::cerr << "BC_JA " << index << std::endl;
				continue;
				break;
			case BC_IFICMPNE:
				v1 = stack.top();
				stack.pop();
				v2 = stack.top();
				stack.push(v1);
				std::cerr << "BC_IFICMPNE " << v1.i << " != " << v2.i << std::endl;
				if (v1.i != v2.i) {
					index += code->getInt16(index + 1) + 1;
					std::cerr << "\tjumping to " << index << std::endl;
					continue;
				}
				break;
			case BC_IFICMPE:
				v1 = stack.top();
				stack.pop();
				v2 = stack.top();
				stack.push(v1);
				std::cerr << "BC_IFICMPE " << v1.i << " == " << v2.i << std::endl;
				if (v1.i == v2.i) {
					index += code->getInt16(index + 1) + 1;
					std::cerr << "\tjumping to " << index << std::endl;
					continue;
				}
				break;
			case BC_IFICMPG:
				v1 = stack.top();
				stack.pop();
				v2 = stack.top();
				stack.push(v1);
				std::cerr << "BC_IFICMPG " << v1.i << " > " << v2.i << std::endl;
				if (v1.i > v2.i) {
					index += code->getInt16(index + 1) + 1;
					std::cerr << "\tjumping to " << index << std::endl;
					continue;
				}
				break;
			case BC_IFICMPGE:
				v1 = stack.top();
				stack.pop();
				v2 = stack.top();
				stack.push(v1);
				std::cerr << "BC_IFICMPGE " << v1.i << " >= " << v2.i << std::endl;
				if (v1.i >= v2.i) {
					index += code->getInt16(index + 1) + 1;
					std::cerr << "\tjumping to " << index << std::endl;
					continue;
				}
				break;
			case BC_IFICMPL:
				v1 = stack.top();
				stack.pop();
				v2 = stack.top();
				stack.push(v1);
				std::cerr << "BC_IFICMPL " << v1.i << " < " << v2.i << std::endl;
				if (v1.i < v2.i) {
					index += code->getInt16(index + 1) + 1;
					std::cerr << "\tjumping to " << index << std::endl;
					continue;
				}
				break;
			case BC_IFICMPLE:
				v1 = stack.top();
				stack.pop();
				v2 = stack.top();
				stack.push(v1);
				std::cerr << "BC_IFICMPL " << v1.i << " <= " << v2.i << std::endl;
				if (v1.i <= v2.i) {
					index += code->getInt16(index + 1) + 1;
					std::cerr << "\tjumping to " << index << std::endl;
					continue;
				}
				break;
			default:
				std::cout << "Unknown instruction " << insn << std::endl;
				break;
		}
		index += bclen(insn);
		//std::cout << "Index increased " << index << std::endl;
	}
}

} /* namespace mathvm */
