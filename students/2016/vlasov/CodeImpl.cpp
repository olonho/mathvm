//
// Created by svloyso on 26.11.16.
//
#include "CodeImpl.h"
#include "Stack.h"

namespace mathvm {

Status* CodeImpl::execute(vector<Var*>& vars) {
	TranslatedFunction* top = functionByName("<top>");
	if(top == nullptr) {
		return Status::Error("Function \"<top>\" not found");
	}

	Context ctx;
	ctx[0] = std::stack<std::map<uint16_t, Var*>>();
	ctx[0].push(std::map<uint16_t, Var*>());
	try {
		execute(top, ctx);
	} catch (Status* s) {
		return s;
	}
	return Status::Ok();
}


Var* CodeImpl::execute(TranslatedFunction* func, Context& ctx){
	if(func == nullptr) {
		throw Status::Error("Unknown function");
	}
	BytecodeFunction* bfunc = dynamic_cast<BytecodeFunction*>(func);
	Stack s;
	Bytecode* bc = bfunc->bytecode();

	int64_t ival = 0;
	double dval = 0;
	uint16_t id = 0;
	uint16_t fid = 0;
	const char* sval = "";
	Var* res = nullptr;
	BytecodeFunction* call = nullptr;
	for(uint32_t bci = 0; bci < bc->length();) {
		size_t length;
		Instruction insn = bc->getInsn(bci);
		bytecodeName(insn, &length);
		switch(insn) {
			case BC_INVALID:
				throw Status::Error("Invalid instruction");
			case BC_DLOAD:
				s.dpush(bc->getDouble(bci + 1));
				break;
			case BC_ILOAD:
				s.ipush(bc->getInt64(bci + 1));
				break;
			case BC_SLOAD:
				s.spush(bc->getUInt16(bci + 1));
				break;
			case BC_DLOAD0:
				s.dpush(0);
				break;
			case BC_ILOAD0:
				s.ipush(0);
				break;
			case BC_SLOAD0:
				s.spush(0);
				break;
			case BC_DLOAD1:
				s.dpush(1);
				break;
			case BC_ILOAD1:
				s.ipush(1);
				break;
			case BC_DLOADM1:
				s.dpush(-1);
				break;
			case BC_ILOADM1:
				s.ipush(-1);
				break;
			case BC_DADD:
				s.dadd();
				break;
			case BC_IADD:
				s.iadd();
				break;
			case BC_DSUB:
				s.dsub();
				break;
			case BC_ISUB:
				s.isub();
				break;
			case BC_DMUL:
				s.dmul();
				break;
			case BC_IMUL:
				s.imul();
				break;
			case BC_DDIV:
				s.ddiv();
				break;
			case BC_IDIV:
				s.idiv();
				break;
			case BC_IMOD:
				s.imod();
				break;
			case BC_DNEG:
				s.dneg();
				break;
			case BC_INEG:
				s.ineg();
				break;
			case BC_IAOR:
				s.ior();
				break;
			case BC_IAAND:
				s.iand();
				break;
			case BC_IAXOR:
				s.ixor();
				break;
			case BC_IPRINT:
				std::cout << s.itop();
				s.pop();
				break;
			case BC_DPRINT:
				std::cout << s.dtop();
				s.pop();
				break;
			case BC_SPRINT:
				std::cout << constantById(s.stop());
				s.pop();
				break;
			case BC_I2D:
				s.i2d();
				break;
			case BC_D2I:
				s.d2i();
				break;
			case BC_S2I:
				id = s.stop();
				s.pop();
				s.ipush(atoi(constantById(id).c_str()));
				break;
			case BC_SWAP:
				s.swap();
				break;
			case BC_POP:
				s.pop();
				break;
			case BC_LOADDVAR0:
			case BC_LOADDVAR1:
			case BC_LOADDVAR2:
			case BC_LOADDVAR3:
			case BC_LOADIVAR0:
			case BC_LOADIVAR1:
			case BC_LOADIVAR2:
			case BC_LOADIVAR3:
			case BC_LOADSVAR0:
			case BC_LOADSVAR1:
			case BC_LOADSVAR2:
			case BC_LOADSVAR3:
			case BC_STOREDVAR0:
			case BC_STOREDVAR1:
			case BC_STOREDVAR2:
			case BC_STOREDVAR3:
			case BC_STOREIVAR0:
			case BC_STOREIVAR1:
			case BC_STOREIVAR2:
			case BC_STOREIVAR3:
			case BC_STORESVAR0:
			case BC_STORESVAR1:
			case BC_STORESVAR2:
			case BC_STORESVAR3:
			case BC_LOADDVAR:
			case BC_LOADIVAR:
			case BC_LOADSVAR:
			case BC_STOREDVAR:
			case BC_STOREIVAR:
			case BC_STORESVAR:
				throw Status::Error("Not implemented instruction");
			case BC_LOADCTXDVAR:
				fid = bc->getUInt16(bci + 1);
				id = bc->getUInt16(bci + 3);
				if(ctx.find(fid) == ctx.end() || ctx[fid].top().find(id) == ctx[fid].top().end()) {
					dval = 0;
				} else {
					dval = ctx[fid].top()[id]->getDoubleValue();
				}
				s.dpush(dval);
				break;
			case BC_LOADCTXIVAR:
				fid = bc->getUInt16(bci + 1);
				id = bc->getUInt16(bci + 3);
				if(ctx.find(fid) == ctx.end() || ctx[fid].top().find(id) == ctx[fid].top().end()) {
					ival = 0;
				} else {
					ival = ctx[fid].top()[id]->getIntValue();
				}
				s.ipush(ival);
				break;
			case BC_LOADCTXSVAR:
				fid = bc->getUInt16(bci + 1);
				id = bc->getUInt16(bci + 3);
				if(ctx.find(fid) == ctx.end() || ctx[fid].top().find(id) == ctx[fid].top().end()) {
					sval = "";
				} else {
					sval = ctx[fid].top()[id]->getStringValue();
				}
				s.spush(makeStringConstant(sval));
				break;
			case BC_STORECTXDVAR:
				fid = bc->getUInt16(bci + 1);
				id = bc->getUInt16(bci + 3);
				if(ctx[fid].top().find(id) == ctx[fid].top().end()) {
					ctx[fid].top()[id] = new Var(VT_DOUBLE, std::to_string(id));
				}
				ctx[fid].top()[id]->setDoubleValue(s.dtop());
				s.pop();
				break;
			case BC_STORECTXIVAR:
				fid = bc->getUInt16(bci + 1);
				id = bc->getUInt16(bci + 3);
				if(ctx[fid].top().find(id) == ctx[fid].top().end()) {
					ctx[fid].top()[id] = new Var(VT_INT, std::to_string(id));
				}
				ctx[fid].top()[id]->setIntValue(s.itop());
				s.pop();
				break;
			case BC_STORECTXSVAR:
				fid = bc->getUInt16(bci + 1);
				id = bc->getUInt16(bci + 3);
				if(ctx[fid].top().find(id) == ctx[fid].top().end()) {
					ctx[fid].top()[id] = new Var(VT_STRING, std::to_string(id));
				}
				ctx[fid].top()[id]->setStringValue(constantById(s.stop()).c_str());
				s.pop();
				break;
			case BC_DCMP:
			case BC_ICMP:
				s.cmp();
				break;
			case BC_JA:
				bci += bc->getInt16(bci + 1) + 1;
				length = 0;
				break;
			case BC_IFICMPNE:
				s.cmp();
				if(s.itop() != 0) {
					bci += bc->getInt16(bci + 1) + 1;
					length = 0;
				}
				s.pop();
				break;
			case BC_IFICMPE:
				s.cmp();
				if(s.itop() == 0) {
					bci += bc->getInt16(bci + 1) + 1;
					length = 0;
				}
				s.pop();
				break;
			case BC_IFICMPG:
				s.cmp();
				if(s.itop() > 0) {
					bci += bc->getInt16(bci + 1) + 1;
					length = 0;
				}
				s.pop();
				break;
			case BC_IFICMPGE:
				s.cmp();
				if(s.itop() >= 0) {
					bci += bc->getInt16(bci + 1) + 1;
					length = 0;
				}
				s.pop();
				break;
			case BC_IFICMPL:
				s.cmp();
				if(s.itop() < 0) {
					bci += bc->getInt16(bci + 1) + 1;
					length = 0;
				}
				s.pop();
				break;
			case BC_IFICMPLE:
				s.cmp();
				if(s.itop() <= 0) {
					bci += bc->getInt16(bci + 1) + 1;
					length = 0;
				}
				s.pop();
				break;
			case BC_DUMP:
				throw Status::Error("Non implemented instruction");
			case BC_STOP:
				bci = bc->length();
				break;
			case BC_CALL:
				fid = bc->getUInt16(bci + 1);
				call = dynamic_cast<BytecodeFunction*>(functionById(fid));
				if(ctx.find(fid) == ctx.end()) {
					ctx[fid] = std::stack<std::map<uint16_t, Var*>>();
				}
				ctx[fid].push(std::map<uint16_t, Var*>());
				for(int i = call->parametersNumber() - 1; i >= 0; --i) {
					ctx[fid].top()[i] = new Var(call->parameterType(i), to_string(i));
					switch(call->parameterType(i)) {
						case VT_INVALID:
						case VT_VOID:
							throw Status::Error("Invalid parameter type");
						case VT_DOUBLE:
							dval = s.dtop();
							ctx[fid].top()[i]->setDoubleValue(dval);
							break;
						case VT_INT:
							ival = s.itop();
							ctx[fid].top()[i]->setIntValue(ival);
							break;
						case VT_STRING:
							sval = constantById(s.stop()).c_str();
							ctx[fid].top()[i]->setStringValue(sval);
							break;
					}
					s.pop();
				}
				res = execute(functionById(fid), ctx);
				if(res) {
					if (res->type() == VT_DOUBLE) s.dpush(res->getDoubleValue());
					if (res->type() == VT_INT) s.ipush(res->getIntValue());
					if (res->type() == VT_STRING) s.spush(makeStringConstant(res->getStringValue()));
				}
				break;
			case BC_CALLNATIVE:
				throw Status::Error("Non implemented instruction");
			case BC_RETURN:
				if(s.empty()) return nullptr;
				res = new Var(s.ttop() == 'i' ? VT_INT : s.ttop() == 'd' ? VT_DOUBLE : VT_STRING, "res");
				if(res->type() == VT_INT) res->setIntValue(s.itop());
				if(res->type() == VT_DOUBLE) res->setDoubleValue(s.dtop());
				if(res->type() == VT_STRING) {
					res->setStringValue(constantById(s.stop()).c_str());
				}
				return res;
			case BC_BREAK:
			case BC_LAST:
				throw Status::Error("Non implemented instruction");
		}

		bci += length;
	}
	return nullptr;
}


} // namespace mathvm
