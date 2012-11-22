/*
 * BytecodeImpl.cpp
 *
 *  Created on: 24.10.2012
 *      Author: temp1ar
 */

#include "BytecodeImpl.h"
#include <stack>

using namespace std;
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

	vector<uint64_t> storedInts(4);
    vector<double> storedDouble(4);

	for(uint32_t index = 0; index < code->length();) {
		Instruction insn = code->getInsn(index);
		value v1;
		value v2;
		value v;
		value res;
        pair<uint16_t, uint16_t> key;
        int16_t offset;

		switch(insn) {
			case BC_INVALID:
				cerr << "INVALID INSTRUCTION" << endl;
				return;
				break;
			case BC_ILOAD:
				v.i = code->getInt64(index + 1);
				cerr << "BC_ILOAD " << v.i << endl;
				stack_.push_back(v);
				break;
            case BC_DLOAD:
                v.d = code->getDouble(index + 1);
                cerr << "BC_DLOAD " << v.d << endl;
                stack_.push_back(v);
                break;
            case BC_SLOAD:
                v.sId = code->getUInt16(index + 1);
                cerr << "BC_SLOAD " << constantById(v.sId) << endl;
                stack_.push_back(v);
                break;
			case BC_ILOAD0:
				v.i = 0;
				cerr << "BC_ILOAD0 " << endl;
				stack_.push_back(v);
				break;
			case BC_ILOAD1:
				v.i = 1;
				cerr << "BC_ILOAD1 " << endl;
				stack_.push_back(v);
				break;
			case BC_ILOADM1:
				v.i = -1;
				cerr << "BC_ILOADM1 " << endl;
				stack_.push_back(v);
				break;
			case BC_IADD:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.pop_back();
				cerr << "BC_IADD " << v1.i << " " << v2.i << endl;
				res.i = v1.i + v2.i;
				stack_.push_back(res);
				break;
            case BC_DADD:
                v1 = stack_.back();
                stack_.pop_back();
                v2 = stack_.back();
                stack_.pop_back();
                cerr << "BC_DADD " << v1.d << " " << v2.d << endl;
                res.d = v1.d + v2.d;
                stack_.push_back(res);
                break;
			case BC_ISUB:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.pop_back();
				res.i = v1.i - v2.i;
				cerr << "BC_ISUB " << v1.i << " " << v2.i << endl;
				stack_.push_back(res);
				break;
            case BC_DSUB:
                v1 = stack_.back();
                stack_.pop_back();
                v2 = stack_.back();
                stack_.pop_back();
                res.d = v1.d - v2.d;
                cerr << "BC_DSUB " << v1.d << " - " << v2.d << endl;
                stack_.push_back(res);
                break;
			case BC_IMUL:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.pop_back();
				cerr << "BC_IMUL " << v1.i << " " << v2.i << endl;
				res.i = v1.i * v2.i;
				stack_.push_back(res);
				break;
            case BC_DMUL:
                v1 = stack_.back();
                stack_.pop_back();
                v2 = stack_.back();
                stack_.pop_back();
                cerr << "BC_IMUL " << v1.d << " " << v2.d << endl;
                res.d = v1.d * v2.d;
                stack_.push_back(res);
                break;
			case BC_IDIV:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.pop_back();
				cerr << "BC_IDIV " << v1.i << " " << v2.i << endl;
				res.i = v1.i / v2.i;
				stack_.push_back(res);
				break;
            case BC_DDIV:
                v1 = stack_.back();
                stack_.pop_back();
                v2 = stack_.back();
                stack_.pop_back();
                cerr << "BC_IDIV " << v1.d << " " << v2.d << endl;
                res.d = v1.d / v2.d;
                stack_.push_back(res);
                break;
			case BC_IMOD:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.pop_back();
				cerr << "BC_IMOD " << v1.i << " " << v2.i << endl;
				res.i = v1.i % v2.i;
				stack_.push_back(res);
				break;
			case BC_INEG:
				v1 = stack_.back();
				stack_.pop_back();
				res.i = -v1.i;
				cerr << "BC_INEG " << endl;
				stack_.push_back(res);
				break;
            case BC_DNEG:
                v1 = stack_.back();
                stack_.pop_back();
                res.d = -v1.d;
                cerr << "BC_DNEG " << endl;
                stack_.push_back(res);
                break;
			case BC_IPRINT:
				v1 = stack_.back();
				stack_.pop_back();
				cerr << "BC_IPRINT " << endl;
				cout << v1.i;
				break;
            case BC_DPRINT:
                v1 = stack_.back();
                stack_.pop_back();
                cerr << "BC_DPRINT " << endl;
                cout << v1.d;
                break;
            case BC_SPRINT:
                v1 = stack_.back();
                stack_.pop_back();
                cerr << "BC_SPRINT " << endl;
                cout << constantById(v1.sId);
                break;
			case BC_SWAP:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.pop_back();
				stack_.push_back(v1);
				stack_.push_back(v2);
				cerr << "BC_SWAP" << endl;
				break;
			case BC_POP:
				stack_.pop_back();
				cerr << "BC_POP" << endl;
				break;
			case BC_LOADIVAR0:
				v.i = storedInts.at(0);
				stack_.push_back(v);
				cerr << "BC_LOADIVAR0" << endl;
				break;
			case BC_LOADIVAR1:
				v.i = storedInts.at(1);
				stack_.push_back(v);
				cerr << "BC_LOADIVAR1" << endl;
				break;
			case BC_LOADIVAR2:
				v.i = storedInts.at(2);
				stack_.push_back(v);
				cerr << "BC_LOADIVAR2" << endl;
				break;
			case BC_LOADIVAR3:
				v.i = storedInts.at(3);
				stack_.push_back(v);
				cerr << "BC_LOADIVAR3" << endl;
				break;
			case BC_STOREIVAR0:
				v = stack_.back();
				storedInts[0] = v.i;
				stack_.pop_back();
				cerr << "BC_STOREIVAR0" << endl;
				break;
			case BC_STOREIVAR1:
				v = stack_.back();
				storedInts[1] = v.i;
				stack_.pop_back();
				cerr << "BC_STOREIVAR1" << endl;
				break;
			case BC_STOREIVAR2:
				v = stack_.back();
				storedInts[2] = v.i;
				stack_.pop_back();
				cerr << "BC_STOREIVAR2" << endl;
				break;
			case BC_STOREIVAR3:
				v = stack_.back();
				storedInts[3] = v.i;
				stack_.pop_back();
				cerr << "BC_STOREIVAR3" << endl;
				break;
			case BC_LOADCTXIVAR:
                key = make_pair(code->getUInt16(index + 1), code->getUInt16(index + 3));
				v.i = storedIntsCustom_[key];
				stack_.push_back(v);
				cerr << "BC_LOADCTXIVAR :" << code->getUInt16(index + 1) << "@" << code->getUInt16(index + 3) << " = " << v.i << endl;
				break;
            case BC_LOADCTXDVAR:
                key = make_pair(code->getUInt16(index + 1), code->getUInt16(index + 3));
                v.d = storedDoublesCustom_[key];
                stack_.push_back(v);
                cerr << "BC_LOADCTXDVAR :" << code->getUInt16(index + 1) << "@" << code->getUInt16(index + 3) << " = " << v.d << endl;
                break;
            case BC_LOADCTXSVAR:
                key = make_pair(code->getUInt16(index + 1), code->getUInt16(index + 3));
                v.sId = storedStringsCustom_[key];
                stack_.push_back(v);
                cerr << "BC_LOADCTXSVAR :" << code->getUInt16(index + 1) << "@" << code->getUInt16(index + 3) << " = " << v.sId << endl;
                break;
			case BC_STORECTXIVAR:
                key = make_pair(code->getUInt16(index + 1), code->getUInt16(index + 3));
                v = stack_.back();
				stack_.pop_back();
				storedIntsCustom_[key] = v.i;
				cerr << "BC_STORECTXIVAR :" << code->getUInt16(index + 1) << "@" << code->getUInt16(index + 3) << " = " << v.i << endl;
				break;
            case BC_STORECTXDVAR:
                key = make_pair(code->getUInt16(index + 1), code->getUInt16(index + 3));
                v = stack_.back();
                stack_.pop_back();
                storedDoublesCustom_[key] = v.d;
                cerr << "BC_STORECTXDVAR :" << code->getUInt16(index + 1) << "@" << code->getUInt16(index + 3) << " = " << v.d << endl;
                break;
            case BC_STORECTXSVAR:
                key = make_pair(code->getUInt16(index + 1), code->getUInt16(index + 3));
                v = stack_.back();
                stack_.pop_back();
                storedStringsCustom_[key] = v.sId;
                cerr << "BC_STORECTXSVAR :" << code->getUInt16(index + 1) << "@" << code->getUInt16(index + 3) << " = " << v.sId << endl;
                break;
			case BC_STOP:
				cout << "Stopping machine" << endl;
				break;
			case BC_JA:
                offset = code->getInt16(index + 1) + 1;
				index += offset;
				cerr << "BC_JA for " << offset << " to " << index << endl;
				continue;
				break;
			case BC_IFICMPNE:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.push_back(v1);
				cerr << "BC_IFICMPNE " << v1.i << " != " << v2.i << endl;
				if (v1.i != v2.i) {
					index += code->getInt16(index + 1) + 1;
					cerr << "\tjumping to " << index << endl;
					continue;
				}
				break;
			case BC_IFICMPE:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.push_back(v1);
				cerr << "BC_IFICMPE " << v1.i << " == " << v2.i << endl;
				if (v1.i == v2.i) {
					index += code->getInt16(index + 1) + 1;
					cerr << "\tjumping to " << index << endl;
					continue;
				}
				break;
			case BC_IFICMPG:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.push_back(v1);
				cerr << "BC_IFICMPG " << v1.i << " > " << v2.i << endl;
				if (v1.i > v2.i) {
					index += code->getInt16(index + 1) + 1;
					cerr << "\tjumping to " << index << endl;
					continue;
				}
				break;
			case BC_IFICMPGE:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.push_back(v1);
				cerr << "BC_IFICMPGE " << v1.i << " >= " << v2.i << endl;
				if (v1.i >= v2.i) {
					index += code->getInt16(index + 1) + 1;
					cerr << "\tjumping to " << index << endl;
					continue;
				}
				break;
			case BC_IFICMPL:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.push_back(v1);
				cerr << "BC_IFICMPL " << v1.i << " < " << v2.i << endl;
				if (v1.i < v2.i) {
					index += code->getInt16(index + 1) + 1;
					cerr << "\tjumping to " << index << endl;
					continue;
				}
				break;
			case BC_IFICMPLE:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.push_back(v1);
				cerr << "BC_IFICMPL " << v1.i << " <= " << v2.i << endl;
				if (v1.i <= v2.i) {
					index += code->getInt16(index + 1) + 1;
					cerr << "\tjumping to " << index << endl;
					continue;
				}
				break;
			case BC_CALL:
                executeFunction((BytecodeFunction*)functionById(code->getUInt16(index + 1)));
				break;
            case BC_RETURN:
                break;
			default:
				cout << "Unknown instruction " << insn << endl;
				break;
		}
		index += bclen(insn);
	}
}

} /* namespace mathvm */