/*
 * BytecodeImpl.cpp
 *
 *  Created on: 24.10.2012
 *      Author: temp1ar
 */

#include "BytecodeImpl.h"
#include <stack>

#define DEBUG

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

	map<uint16_t, uint64_t> storedInts;
    map<uint16_t, double> storedDouble;
    map<uint16_t, uint16_t> storedStrings;

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
				#ifdef DEBUG
				cerr << "INVALID INSTRUCTION" << endl;
				#endif // DEBUG
				return;
				break;
			case BC_ILOAD:
				v.i = code->getInt64(index + 1);
				#ifdef DEBUG
				cerr << "BC_ILOAD " << v.i << endl;
				#endif // DEBUG
				stack_.push_back(v);
				break;
            case BC_DLOAD:
                v.d = code->getDouble(index + 1);
                #ifdef DEBUG
				cerr << "BC_DLOAD " << v.d << endl;
				#endif // DEBUG
                stack_.push_back(v);
                break;
            case BC_SLOAD:
                v.sId = code->getUInt16(index + 1);
                #ifdef DEBUG
				cerr << "BC_SLOAD " << constantById(v.sId) << endl;
				#endif // DEBUG
                stack_.push_back(v);
                break;
			case BC_ILOAD0:
				v.i = 0;
				#ifdef DEBUG
				cerr << "BC_ILOAD0 " << endl;
				#endif // DEBUG
				stack_.push_back(v);
				break;
			case BC_ILOAD1:
				v.i = 1;
				#ifdef DEBUG
				cerr << "BC_ILOAD1 " << endl;
				#endif // DEBUG
				stack_.push_back(v);
				break;
			case BC_ILOADM1:
				v.i = -1;
				#ifdef DEBUG
				cerr << "BC_ILOADM1 " << endl;
				#endif // DEBUG
				stack_.push_back(v);
				break;
			case BC_IADD:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.pop_back();
				#ifdef DEBUG
				cerr << "BC_IADD " << v1.i << " " << v2.i << endl;
				#endif // DEBUG
				res.i = v1.i + v2.i;
				stack_.push_back(res);
				break;
            case BC_DADD:
                v1 = stack_.back();
                stack_.pop_back();
                v2 = stack_.back();
                stack_.pop_back();
                #ifdef DEBUG
				cerr << "BC_DADD " << v1.d << " " << v2.d << endl;
				#endif // DEBUG
                res.d = v1.d + v2.d;
                stack_.push_back(res);
                break;
			case BC_ISUB:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.pop_back();
				res.i = v1.i - v2.i;
				#ifdef DEBUG
				cerr << "BC_ISUB " << v1.i << " " << v2.i << endl;
				#endif // DEBUG
				stack_.push_back(res);
				break;
            case BC_DSUB:
                v1 = stack_.back();
                stack_.pop_back();
                v2 = stack_.back();
                stack_.pop_back();
                res.d = v1.d - v2.d;
                #ifdef DEBUG
				cerr << "BC_DSUB " << v1.d << " - " << v2.d << endl;
				#endif // DEBUG
                stack_.push_back(res);
                break;
			case BC_IMUL:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.pop_back();
				#ifdef DEBUG
				cerr << "BC_IMUL " << v1.i << " " << v2.i << endl;
				#endif // DEBUG
				res.i = v1.i * v2.i;
				stack_.push_back(res);
				break;
            case BC_DMUL:
                v1 = stack_.back();
                stack_.pop_back();
                v2 = stack_.back();
                stack_.pop_back();
                #ifdef DEBUG
				cerr << "BC_IMUL " << v1.d << " " << v2.d << endl;
				#endif // DEBUG
                res.d = v1.d * v2.d;
                stack_.push_back(res);
                break;
			case BC_IDIV:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.pop_back();
				#ifdef DEBUG
				cerr << "BC_IDIV " << v1.i << " " << v2.i << endl;
				#endif // DEBUG
				res.i = v1.i / v2.i;
				stack_.push_back(res);
				break;
            case BC_DDIV:
                v1 = stack_.back();
                stack_.pop_back();
                v2 = stack_.back();
                stack_.pop_back();
                #ifdef DEBUG
				cerr << "BC_IDIV " << v1.d << " " << v2.d << endl;
				#endif // DEBUG
                res.d = v1.d / v2.d;
                stack_.push_back(res);
                break;
			case BC_IMOD:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.pop_back();
				#ifdef DEBUG
				cerr << "BC_IMOD " << v1.i << " " << v2.i << endl;
				#endif // DEBUG
				res.i = v1.i % v2.i;
				stack_.push_back(res);
				break;
			case BC_INEG:
				v1 = stack_.back();
				stack_.pop_back();
				res.i = -v1.i;
				#ifdef DEBUG
				cerr << "BC_INEG " << endl;
				#endif // DEBUG
				stack_.push_back(res);
				break;
            case BC_DNEG:
                v1 = stack_.back();
                stack_.pop_back();
                res.d = -v1.d;
                #ifdef DEBUG
				cerr << "BC_DNEG " << endl;
				#endif // DEBUG
                stack_.push_back(res);
                break;
			case BC_IPRINT:
				v1 = stack_.back();
				stack_.pop_back();
				#ifdef DEBUG
				cerr << "BC_IPRINT " << endl;
				#endif // DEBUG
				cout << v1.i;
				break;
            case BC_DPRINT:
                v1 = stack_.back();
                stack_.pop_back();
                #ifdef DEBUG
				cerr << "BC_DPRINT " << endl;
				#endif // DEBUG
                cout << v1.d;
                break;
            case BC_SPRINT:
                v1 = stack_.back();
                stack_.pop_back();
                #ifdef DEBUG
				cerr << "BC_SPRINT " << endl;
				#endif // DEBUG
                cout << constantById(v1.sId);
                break;
			case BC_SWAP:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.pop_back();
				stack_.push_back(v1);
				stack_.push_back(v2);
				#ifdef DEBUG
				cerr << "BC_SWAP" << endl;
				#endif // DEBUG
				break;
			case BC_POP:
				stack_.pop_back();
				#ifdef DEBUG
				cerr << "BC_POP" << endl;
				#endif // DEBUG
				break;
			case BC_LOADIVAR0:
				v.i = storedInts.at(0);
				stack_.push_back(v);
				#ifdef DEBUG
				cerr << "BC_LOADIVAR0" << endl;
				#endif // DEBUG
				break;
			case BC_LOADIVAR1:
				v.i = storedInts.at(1);
				stack_.push_back(v);
				#ifdef DEBUG
				cerr << "BC_LOADIVAR1" << endl;
				#endif // DEBUG
				break;
			case BC_LOADIVAR2:
				v.i = storedInts.at(2);
				stack_.push_back(v);
				#ifdef DEBUG
				cerr << "BC_LOADIVAR2" << endl;
				#endif // DEBUG
				break;
			case BC_LOADIVAR3:
				v.i = storedInts.at(3);
				stack_.push_back(v);
				#ifdef DEBUG
				cerr << "BC_LOADIVAR3" << endl;
				#endif // DEBUG
				break;
			case BC_STOREIVAR0:
				v = stack_.back();
				storedInts[0] = v.i;
				stack_.pop_back();
				#ifdef DEBUG
				cerr << "BC_STOREIVAR0" << endl;
				#endif // DEBUG
				break;
			case BC_STOREIVAR1:
				v = stack_.back();
				storedInts[1] = v.i;
				stack_.pop_back();
				#ifdef DEBUG
				cerr << "BC_STOREIVAR1" << endl;
				#endif // DEBUG
				break;
			case BC_STOREIVAR2:
				v = stack_.back();
				storedInts[2] = v.i;
				stack_.pop_back();
				#ifdef DEBUG
				cerr << "BC_STOREIVAR2" << endl;
				#endif // DEBUG
				break;
			case BC_STOREIVAR3:
				v = stack_.back();
				storedInts[3] = v.i;
				stack_.pop_back();
				#ifdef DEBUG
				cerr << "BC_STOREIVAR3" << endl;
				#endif // DEBUG
				break;
            case BC_STOREIVAR:
                v = stack_.back();
                storedInts[code->getUInt16(index + 1)] = v.i;
                stack_.pop_back();
                #ifdef DEBUG
				cerr << "BC_STOREIVAR " <<  v.i << endl;
				#endif // DEBUG
                break;
            case BC_LOADIVAR:
                v.i = storedInts.at(code->getUInt16(index + 1));
                stack_.push_back(v);
                #ifdef DEBUG
				cerr << "BC_LOADIVAR " << v.i << endl;
				#endif // DEBUG
                break;
            case BC_STOREDVAR:
                v = stack_.back();
                storedDouble[code->getUInt16(index + 1)] = v.d;
                stack_.pop_back();
                #ifdef DEBUG
				cerr << "BC_STOREDVAR " <<  v.d << endl;
				#endif // DEBUG
                break;
            case BC_LOADDVAR:
                v.d = storedDouble.at(code->getUInt16(index + 1));
                stack_.push_back(v);
                #ifdef DEBUG
				cerr << "BC_LOADDVAR " << v.d << endl;
				#endif // DEBUG
                break;
            case BC_STORESVAR:
                v = stack_.back();
                stack_.pop_back();
                storedStrings[code->getUInt16(index + 1)] = v.sId;
                #ifdef DEBUG
				cerr << "BC_STORESVAR :" << code->getUInt16(index + 1) << " = " << v.sId << endl;
				#endif // DEBUG
                break;
            case BC_LOADSVAR:
                v.sId = storedStrings[code->getUInt16(index + 1)];
                stack_.push_back(v);
                #ifdef DEBUG
				cerr << "BC_LOADSVAR :" << code->getUInt16(index + 1) << " = " << v.sId << endl;
				#endif // DEBUG
                break;			
			case BC_STOP:
				cout << "Stopping machine" << endl;
				break;
			case BC_JA:
                offset = code->getInt16(index + 1) + 1;
				index += offset;
				#ifdef DEBUG
				cerr << "BC_JA for " << offset << " to " << index << endl;
				#endif // DEBUG
				continue;
				break;
			case BC_IFICMPNE:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.push_back(v1);
				#ifdef DEBUG
				cerr << "BC_IFICMPNE " << v1.i << " != " << v2.i << endl;
				#endif // DEBUG
				if (v1.i != v2.i) {
					index += code->getInt16(index + 1) + 1;
					#ifdef DEBUG
				cerr << "\tjumping to " << index << endl;
				#endif // DEBUG
					continue;
				}
				break;
			case BC_IFICMPE:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.push_back(v1);
				#ifdef DEBUG
				cerr << "BC_IFICMPE " << v1.i << " == " << v2.i << endl;
				#endif // DEBUG
				if (v1.i == v2.i) {
					index += code->getInt16(index + 1) + 1;
					#ifdef DEBUG
				cerr << "\tjumping to " << index << endl;
				#endif // DEBUG
					continue;
				}
				break;
			case BC_IFICMPG:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.push_back(v1);
				#ifdef DEBUG
				cerr << "BC_IFICMPG " << v1.i << " > " << v2.i << endl;
				#endif // DEBUG
				if (v1.i > v2.i) {
					index += code->getInt16(index + 1) + 1;
					#ifdef DEBUG
				cerr << "\tjumping to " << index << endl;
				#endif // DEBUG
					continue;
				}
				break;
			case BC_IFICMPGE:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.push_back(v1);
				#ifdef DEBUG
				cerr << "BC_IFICMPGE " << v1.i << " >= " << v2.i << endl;
				#endif // DEBUG
				if (v1.i >= v2.i) {
					index += code->getInt16(index + 1) + 1;
					#ifdef DEBUG
				cerr << "\tjumping to " << index << endl;
				#endif // DEBUG
					continue;
				}
				break;
			case BC_IFICMPL:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.push_back(v1);
				#ifdef DEBUG
				cerr << "BC_IFICMPL " << v1.i << " < " << v2.i << endl;
				#endif // DEBUG
				if (v1.i < v2.i) {
					index += code->getInt16(index + 1) + 1;
					#ifdef DEBUG
				cerr << "\tjumping to " << index << endl;
				#endif // DEBUG
					continue;
				}
				break;
			case BC_IFICMPLE:
				v1 = stack_.back();
				stack_.pop_back();
				v2 = stack_.back();
				stack_.push_back(v1);
				#ifdef DEBUG
				cerr << "BC_IFICMPL " << v1.i << " <= " << v2.i << endl;
				#endif // DEBUG
				if (v1.i <= v2.i) {
					index += code->getInt16(index + 1) + 1;
					#ifdef DEBUG
				cerr << "\tjumping to " << index << endl;
				#endif // DEBUG
					continue;
				}
				break;
			case BC_CALL:
                executeFunction((BytecodeFunction*)functionById(code->getUInt16(index + 1)));
				break;
            case BC_RETURN:
                return;
                break;
			default:
				cerr << "Unknown instruction " << insn << endl;
				break;
		}
		index += bclen(insn);
	}
}

} /* namespace mathvm */
