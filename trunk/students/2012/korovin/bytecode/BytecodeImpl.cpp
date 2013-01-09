/*
 * BytecodeImpl.cpp
 *
 *  Created on: 24.10.2012
 *      Author: temp1ar
 */

#include "BytecodeImpl.h"
#include <stack>

//#define DEBUG

using namespace std;
namespace mathvm {

BytecodeImpl::BytecodeImpl() {
	context_ = 0;
}

BytecodeImpl::~BytecodeImpl() {
}

Status* BytecodeImpl::execute(vector<Var*>& vars) {
	BytecodeFunction* func = (BytecodeFunction*) functionByName("<top>");
	executeFunction(func);
	return 0;
}

static size_t bclen(Instruction insn) {
	static const struct {
		const char* name;
		Instruction insn;
		size_t length;
	} names[] = {
#define BC_NAME(b, d, l) {#b, BC_##b, l},
			FOR_BYTECODES(BC_NAME) };

	if (insn >= BC_INVALID && insn < BC_LAST)
		return names[insn].length;

	assert(false);
	return 0;
}

void BytecodeImpl::executeFunction(BytecodeFunction* f) {
	Bytecode* code = f->bytecode();
	Context* oldContext = context_;
	context_ = new Context(context_, f);

	for (uint32_t index = 0; index < code->length();) {
		Instruction insn = code->getInsn(index);
		value v1;
		value v2;
		value v;
		value res;
		pair<uint16_t, uint16_t> key;
		int16_t offset;

		switch (insn) {
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
			v.sPtr = constantById(code->getUInt16(index + 1)).c_str();
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
			cout << v1.sPtr;
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
			v = context_->vars()[0];
			stack_.push_back(v);
#ifdef DEBUG
			cerr << "BC_LOADIVAR0" << endl;
#endif // DEBUG
			break;
		case BC_LOADIVAR1:
			v = context_->vars()[1];
			stack_.push_back(v);
#ifdef DEBUG
			cerr << "BC_LOADIVAR1" << endl;
#endif // DEBUG
			break;
		case BC_LOADIVAR2:
			v = context_->vars()[2];
			stack_.push_back(v);
#ifdef DEBUG
			cerr << "BC_LOADIVAR2" << endl;
#endif // DEBUG
			break;
		case BC_LOADIVAR3:
			v = context_->vars()[3];
			stack_.push_back(v);
#ifdef DEBUG
			cerr << "BC_LOADIVAR3" << endl;
#endif // DEBUG
			break;
		case BC_STOREIVAR0:
			v = stack_.back();
			context_->vars()[0] = v;
			stack_.pop_back();
#ifdef DEBUG
			cerr << "BC_STOREIVAR0" << endl;
#endif // DEBUG
			break;
		case BC_STOREIVAR1:
			v = stack_.back();
			context_->vars()[1] = v;
			stack_.pop_back();
#ifdef DEBUG
			cerr << "BC_STOREIVAR1" << endl;
#endif // DEBUG
			break;
		case BC_STOREIVAR2:
			v = stack_.back();
			context_->vars()[2] = v;
			stack_.pop_back();
#ifdef DEBUG
			cerr << "BC_STOREIVAR2" << endl;
#endif // DEBUG
			break;
		case BC_STOREIVAR3:
			v = stack_.back();
			context_->vars()[3] = v;
			stack_.pop_back();
#ifdef DEBUG
			cerr << "BC_STOREIVAR3" << endl;
#endif // DEBUG
			break;
		case BC_STOREIVAR:
			v = stack_.back();
			context_->vars()[code->getUInt16(index + 1)] = v;
			stack_.pop_back();
#ifdef DEBUG
			cerr << "BC_STOREIVAR " << v.i << endl;
#endif // DEBUG
			break;
		case BC_LOADIVAR:
			v = context_->vars()[code->getUInt16(index + 1)];
			stack_.push_back(v);
#ifdef DEBUG
			cerr << "BC_LOADIVAR " << v.i << endl;
#endif // DEBUG
			break;
		case BC_STOREDVAR:
			v = stack_.back();
			context_->vars()[code->getUInt16(index + 1)] = v;
			stack_.pop_back();
#ifdef DEBUG
			cerr << "BC_STOREDVAR " << v.d << endl;
#endif // DEBUG
			break;
		case BC_LOADDVAR:
			v = context_->vars()[code->getUInt16(index + 1)];
			stack_.push_back(v);
#ifdef DEBUG
			cerr << "BC_LOADDVAR " << v.d << endl;
#endif // DEBUG
			break;
		case BC_STORESVAR:
			v = stack_.back();
			stack_.pop_back();
			context_->vars()[code->getUInt16(index + 1)] = v;
#ifdef DEBUG
			cerr << "BC_STORESVAR :" << code->getUInt16(index + 1) << " = " << v.sId << endl;
#endif // DEBUG
			break;
		case BC_LOADSVAR:
			v = context_->vars()[code->getUInt16(index + 1)];
			stack_.push_back(v);
#ifdef DEBUG
			cerr << "BC_LOADSVAR :" << code->getUInt16(index + 1) << " = " << v.sId << endl;
#endif // DEBUG
			break;
		case BC_STORECTXIVAR:
			v = stack_.back();
			context_->vars(code->getUInt16(index + 1))[code->getUInt16(
					index + 3)] = v;
			stack_.pop_back();
#ifdef DEBUG
			cerr << "BC_STOREIVAR " << v.i << endl;
#endif // DEBUG
			break;
		case BC_LOADCTXIVAR:
			v = context_->vars(code->getUInt16(index + 1))[code->getUInt16(
					index + 3)];
			stack_.push_back(v);
#ifdef DEBUG
			cerr << "BC_LOADIVAR " << v.i << endl;
#endif // DEBUG
			break;
		case BC_STORECTXDVAR:
			v = stack_.back();
			context_->vars(code->getUInt16(index + 1))[code->getUInt16(
					index + 3)] = v;
			stack_.pop_back();
#ifdef DEBUG
			cerr << "BC_STOREDVAR " << v.d << endl;
#endif // DEBUG
			break;
		case BC_LOADCTXDVAR:
			v = context_->vars(code->getUInt16(index + 1))[code->getUInt16(
					index + 3)];
			stack_.push_back(v);
#ifdef DEBUG
			cerr << "BC_LOADDVAR " << v.d << endl;
#endif // DEBUG
			break;
		case BC_STORECTXSVAR:
			v = stack_.back();
			stack_.pop_back();
			context_->vars(code->getUInt16(index + 1))[code->getUInt16(
					index + 3)] = v;
#ifdef DEBUG
			cerr << "BC_STORESVAR :" << code->getUInt16(index + 1) << " = " << v.sId << endl;
#endif // DEBUG
			break;
		case BC_LOADCTXSVAR:
			v = context_->vars(code->getUInt16(index + 1))[code->getUInt16(
					index + 3)];
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
			executeFunction(
					(BytecodeFunction*) functionById(
							code->getUInt16(index + 1)));
			break;
		case BC_CALLNATIVE: {
			Signature const *signature;
			void const *native = nativeById(code->getUInt16(index + 1),
					&signature);

			size_t offset = 0;
			static char nativeArgs_[1024];
			int ptrSize = sizeof(char *);
			for (size_t i = 1; i < signature->size(); ++i) {
				value v = stack_.back();
				stack_.pop_back();

				switch (signature->at(i).first) {
				case VT_INT:
					memcpy(nativeArgs_ + offset, &v.i, ptrSize);
					break;
				case VT_DOUBLE:
					memcpy(nativeArgs_ + offset, &v.d, ptrSize);
					break;
				case VT_STRING:
					memcpy(nativeArgs_ + offset, &v.sPtr, ptrSize);
					break;
				default:
					assert(false);
					break;
				}

				offset += ptrSize;
			}

			switch (signature->at(0).first) {
			case VT_DOUBLE: {
				double (*returnsDouble)(char *) =
						function_cast<double (*)(char *)>(native);
				res.d = (*returnsDouble)(nativeArgs_);
				stack_.push_back(res);
				break;
			}
			case VT_STRING: {
				const char* (*returnsCharPtr)(char *) =
						function_cast<const char* (*)(char *)>(native);
				res.sPtr = (*returnsCharPtr)(nativeArgs_);
				stack_.push_back(res);
				break;
			}
			case VT_INT: {
				int (*returnsInt)(char*) =
						function_cast<int (*)(char*)>(native);
				res.i = (*returnsInt)(nativeArgs_);
				stack_.push_back(res);
				break;
			}
			case VT_VOID: {
				void (*returns_void)(char*) =
						function_cast<void (*)(char*)>(native);
				(*returns_void)(nativeArgs_);
				break;
			}
			default:
				break;
			}
			break;
		}
		case BC_RETURN:
			delete context_;
			context_ = oldContext;
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
