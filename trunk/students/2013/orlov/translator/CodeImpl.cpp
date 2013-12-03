#include "CodeImpl.h"
#include <sstream>

namespace mathvm {

Status * CodeImpl::execute(vector<Var *> &vars) {
	currentScope = 0;
	callFunction(0);
	while(true) {
		switch(currentScope->function->bytecode()->getInsn(currentScope->ip++)) {
		case BC_INVALID: { return new Status("Invalid operation"); }
		case BC_DLOAD:   { exec_dload(getVal<double>(8));   break; }
		case BC_ILOAD:   { exec_iload(getVal<int64_t>(8));  break; }
		case BC_SLOAD:   { exec_sload(getVal<uint16_t>(2)); break; }
		case BC_DLOAD0:  { exec_dload(0.0);			  break; }
		case BC_ILOAD0:  { exec_iload((int64_t)0);    break; }
		case BC_SLOAD0:  { exec_sload((int64_t)0);    break; }
		case BC_DLOAD1:  { exec_dload(1.0);			  break; }
		case BC_ILOAD1:  { exec_iload((int64_t)1);    break; }
		case BC_DLOADM1: { exec_dload(-1.0);	      break; }
		case BC_ILOADM1: { exec_iload((int64_t)(-1)); break; }
		case BC_DADD: { doubleBinOp(tADD); break; }
		case BC_DSUB: { doubleBinOp(tSUB); break; }
		case BC_DMUL: { doubleBinOp(tMUL); break; }
		case BC_DDIV: { doubleBinOp(tDIV); break; }
		case BC_DCMP: { doubleBinOp(tEQ);  break; }
		case BC_DNEG: {
			Var v(VT_DOUBLE, "");
			v.setDoubleValue(-popStack().getDoubleValue());
			varStack.push(v);
			break;
		}
		case BC_IADD: { intBinOp(tADD); break; }
		case BC_ISUB: { intBinOp(tSUB); break; }
		case BC_IMUL: { intBinOp(tMUL); break; }
		case BC_IDIV: { intBinOp(tDIV); break; }
		case BC_IMOD: {	intBinOp(tMOD); break; }
		case BC_IAAND:{	intBinOp(tAAND);break; }
		case BC_IAOR: { intBinOp(tAOR); break; }
		case BC_IAXOR:{ intBinOp(tAXOR);break; }
		case BC_ICMP: {	intBinOp(tEQ); 	break; }
		case BC_INEG: {
			Var v(VT_INT, "");
			v.setIntValue(-popStack().getIntValue());
			varStack.push(v);
			break;
		}
		case BC_SWAP: {
			Var a = popStack();
			Var b = popStack();
			varStack.push(a);
			varStack.push(b);
			break;
		}
		case BC_I2D: {
			Var v = popStack();
			Var v1(VT_DOUBLE, "");
			v1.setDoubleValue(v.getIntValue());
			varStack.push(v1);
			break;
		}
		case BC_D2I: {
			Var v = popStack();
			Var v1(VT_INT, "");
			v1.setIntValue(static_cast<int64_t>(v.getDoubleValue()));
			varStack.push(v1);
			break;
		}
		case BC_S2I: {
			Var v = popStack();
			Var v1(VT_INT, "");
			stringstream ss(v.getStringValue());
			uint64_t res;
			ss >> res;
			v1.setIntValue(res);
			varStack.push(v1);
			break;
		}
		case BC_IPRINT: { std::cout << popStack().getIntValue();    break;	}
		case BC_DPRINT: { std::cout << popStack().getDoubleValue(); break;	}
		case BC_SPRINT: { std::cout << popStack().getStringValue(); break;	}
		case BC_LOADIVAR0:
		case BC_LOADDVAR0:
		case BC_LOADSVAR0:
			loadVar(0);
			break;
		case BC_LOADIVAR1:
		case BC_LOADDVAR1:
		case BC_LOADSVAR1:
			loadVar(1);
			break;
		case BC_LOADIVAR2:
		case BC_LOADDVAR2:
		case BC_LOADSVAR2:
			loadVar(2);
			break;
		case BC_LOADIVAR3:
		case BC_LOADDVAR3:
		case BC_LOADSVAR3:
			loadVar(3);
			break;
		case BC_LOADIVAR:
		case BC_LOADDVAR:
		case BC_LOADSVAR:
			loadVar(getVal<uint16_t>(2));
			break;
		case BC_LOADCTXIVAR:
		case BC_LOADCTXDVAR:
		case BC_LOADCTXSVAR: {
			uint16_t a = getVal<uint16_t>(2); 
			uint16_t b = getVal<uint16_t>(2);
			loadVar(a, b); 
			break;
		}
		case BC_STOREIVAR0:
		case BC_STOREDVAR0:
		case BC_STORESVAR0:
			storeVar(0);
			break;
		case BC_STOREIVAR1:
		case BC_STOREDVAR1:
		case BC_STORESVAR1:
			storeVar(1);
			break;
		case BC_STOREIVAR2:
		case BC_STOREDVAR2:
		case BC_STORESVAR2:
			storeVar(2);
			break;
		case BC_STOREIVAR3:
		case BC_STOREDVAR3:
		case BC_STORESVAR3:
			storeVar(3);
			break;
		case BC_STOREIVAR:
		case BC_STOREDVAR:
		case BC_STORESVAR:
			storeVar(getVal<uint16_t>(2));
			break;
		case BC_STORECTXIVAR:
		case BC_STORECTXDVAR:
		case BC_STORECTXSVAR: {
			uint16_t a = getVal<uint16_t>(2); 
			uint16_t b = getVal<uint16_t>(2); 
			storeVar(a, b); 
			break;
		}
		case BC_JA:
			currentScope->ip += currentScope->function->bytecode()->getInt16(currentScope->ip);
			break;
		case BC_IFICMPE: {
			int64_t a = popStack().getIntValue();
			int64_t b = popStack().getIntValue();
			currentScope->ip += (a == b) ? 
				currentScope->function->bytecode()->getInt16(currentScope->ip) : 2;
			break;
		}		
		case BC_IFICMPNE: {
			int64_t a = popStack().getIntValue();
			int64_t b = popStack().getIntValue();
			currentScope->ip += (a != b) ? 
				currentScope->function->bytecode()->getInt16(currentScope->ip) : 2;
			break;
		}
		case BC_IFICMPG: {
			int64_t a = popStack().getIntValue();
			int64_t b = popStack().getIntValue();
			currentScope->ip += (a > b) ? 
				currentScope->function->bytecode()->getInt16(currentScope->ip) : 2;
			break;
		}
		case BC_IFICMPL: {
			int64_t a = popStack().getIntValue();
			int64_t b = popStack().getIntValue();
			currentScope->ip += (a < b) ? 
				currentScope->function->bytecode()->getInt16(currentScope->ip) : 2;
			break;
		}
		case BC_IFICMPGE: {
			int64_t a = popStack().getIntValue();
			int64_t b = popStack().getIntValue();
			currentScope->ip += (a >= b) ? 
				currentScope->function->bytecode()->getInt16(currentScope->ip) : 2;
			break;
		}
		case BC_IFICMPLE: {
			int64_t a = popStack().getIntValue();
			int64_t b = popStack().getIntValue();
			currentScope->ip += (a <= b) ? 
				currentScope->function->bytecode()->getInt16(currentScope->ip) : 2;
			break;
		}
		case BC_CALL:
			callFunction(getVal<uint16_t>(2));
			break;
		case BC_CALLNATIVE:
			callNative(getVal<uint16_t>(2));
			break;
		case BC_RETURN: {
			FuncScope * parent = currentScope->parent;
			delete currentScope;
			currentScope = parent;
			break;
		}		
		case BC_STOP:
			return 0;
		default:
			return new Status("Unknown instruction.");
		}
	}
	return 0;
}

void CodeImpl::callNative(uint16_t id) {
	//TODO
}

void CodeImpl::exec_dload(double val) {
	Var v(VT_DOUBLE, "");
	v.setDoubleValue(val);
	varStack.push(v);
}

void CodeImpl::exec_iload(int64_t val) {
	Var v(VT_INT, "");
	v.setIntValue(val);
	varStack.push(v);
}

void CodeImpl::exec_sload(uint16_t id) {
	Var v(VT_STRING, "");
	v.setStringValue(constantById(id).data());
	varStack.push(v);
}

void CodeImpl::doubleBinOp(TokenKind op) {
	Var a = popStack();
	Var b = popStack();
	Var res(VT_DOUBLE, "");
	switch (op) {
	case tADD:
		res.setDoubleValue(a.getDoubleValue() + b.getDoubleValue());
		break;
	case tSUB:
		res.setDoubleValue(a.getDoubleValue() - b.getDoubleValue());
		break;
	case tMUL:
		res.setDoubleValue(a.getDoubleValue() * b.getDoubleValue());
		break;
	case tDIV:
		res.setDoubleValue(a.getDoubleValue() / b.getDoubleValue());
		break;
	case tEQ:
		res.setDoubleValue((int64_t)(a.getDoubleValue() == b.getDoubleValue()));
		break;
	default:
		throw std::string("Unsupported double operation");
		return;
	}
	varStack.push(res);
}

void CodeImpl::intBinOp(TokenKind op) {
	Var a = popStack();
	Var b = popStack();
	Var res(VT_INT, "");
	switch (op) {
	case tADD:
		res.setIntValue(a.getIntValue() + b.getIntValue());
		break;
	case tSUB:
		res.setIntValue(a.getIntValue() - b.getIntValue());
		break;
	case tMUL:
		res.setIntValue(a.getIntValue() * b.getIntValue());
		break;
	case tDIV:
		res.setIntValue(a.getIntValue() / b.getIntValue());
		break;
	case tEQ:
		res.setIntValue((int64_t)(a.getIntValue() == b.getIntValue()));
		break;
	case tMOD:
		res.setIntValue(a.getIntValue() % b.getIntValue());
		break;
	case tAAND:
		res.setIntValue(a.getIntValue() & b.getIntValue());
		break;
	case tAOR:
		res.setIntValue(a.getIntValue() | b.getIntValue());
		break;
	case tAXOR:
		res.setIntValue(a.getIntValue() ^ b.getIntValue());
		break;
	default:
		throw std::string("Unsupported integer operation");
		return;
	}
	varStack.push(res);
}

Var CodeImpl::popStack() {
	if (varStack.empty())
		throw std::string("Stack is empty");
	Var res = varStack.top();
	varStack.pop();
	return res;
}

void CodeImpl::callFunction(uint16_t id) {
	BytecodeFunction * f = (BytecodeFunction *)functionById(id);
	currentScope = new FuncScope(f, currentScope);
}

void CodeImpl::loadVar(uint16_t idx) {
	varStack.push(currentScope->vars[idx]);
}

void CodeImpl::loadVar(uint16_t cid, uint16_t idx) {
	FuncScope *s = currentScope->parent;
	while (s && s->function->id() != cid) {
		s = s->parent;
	}
	if (!s) throw std::string("context not found");
	varStack.push(s->vars[idx]);
}

void CodeImpl::storeVar(uint16_t idx) {
	currentScope->vars[idx] = popStack();
}

void CodeImpl::storeVar(uint16_t cid, uint16_t idx) {
	FuncScope *s = currentScope->parent;
	while (s && s->function->id() != cid) {
		s = s->parent;
	}
	if (!s) throw std::string("context not found");
	s->vars[idx] = popStack();
}
}