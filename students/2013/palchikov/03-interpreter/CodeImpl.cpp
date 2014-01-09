#include "CodeImpl.h"

Status* CodeImpl::execute(vector<Var*>& vars)
{
	stopped = false;
	initContexts();

	BytecodeFunction* top = (BytecodeFunction*)functionById(0);
	executeBytecode(top->bytecode());

	return new Status();
}

void CodeImpl::initContexts()
{
	Code::FunctionIterator fns(this);
	while (fns.hasNext()) {
		TranslatedFunction* f = fns.next();
		vector<Val> context(f->parametersNumber() + f->localsNumber());
		stack<vector<Val> > contextStack;
		contextStack.push(context);
		contexts.push_back(contextStack);
	}
}

void CodeImpl::executeBytecode(Bytecode* bc)
{
	size_t ip = 0;
	size_t len = bc->length();
	Val val;
	Val val2;
	Val res;
	uint16_t ctxId;
	uint16_t varId;

	while (!stopped && ip < len) {
		Instruction insn = bc->getInsn(ip);
		switch (insn) {
		case BC_ILOAD0:
			val.ival = 0;
			compStack.push(val);
			break;
		case BC_ILOAD1:
			val.ival = 1;
			compStack.push(val);
			break;
		case BC_ILOADM1:
			val.ival = -1;
			compStack.push(val);
			break;
		case BC_ILOAD:
			val.ival = bc->getInt64(ip + 1);
			compStack.push(val);
			ip += sizeof(int64_t);
			break;

		case BC_DLOAD0:
			val.dval = 0.0d;
			compStack.push(val);
			break;
		case BC_DLOAD1:
			val.dval = 1.0d;
			compStack.push(val);
			break;
		case BC_DLOADM1:
			val.dval = -1.0d;
			compStack.push(val);
			break;
		case BC_DLOAD:
			val.dval = bc->getDouble(ip + 1);
			compStack.push(val);
			ip += sizeof(double);
			break;

		case BC_SLOAD:
			val.sval = bc->getUInt16(ip + 1);
			compStack.push(val);
			ip += sizeof(uint16_t);
			break;

		case BC_IADD:
		case BC_DADD:
		case BC_ISUB:
		case BC_DSUB:
		case BC_IMUL:
		case BC_DMUL:
		case BC_IDIV:
		case BC_DDIV:
		case BC_IMOD:
		case BC_IAAND:
		case BC_IAOR:
		case BC_IAXOR:
			val = compStack.top();
			compStack.pop();
			val2 = compStack.top();
			compStack.pop();

			if (insn == BC_IADD) res.ival = val.ival + val2.ival;
			else if (insn == BC_DADD) res.dval = val.dval + val2.dval;
			else if (insn == BC_ISUB) res.ival = val.ival - val2.ival;
			else if (insn == BC_DSUB) res.dval = val.dval - val2.dval;
			else if (insn == BC_IMUL) res.ival = val.ival * val2.ival;
			else if (insn == BC_DMUL) res.dval = val.dval * val2.dval;
			else if (insn == BC_IDIV) res.ival = val.ival / val2.ival;
			else if (insn == BC_DDIV) res.dval = val.dval / val2.dval;
			else if (insn == BC_IMOD) res.ival = val.ival % val2.ival;
			else if (insn == BC_IAAND) res.ival = val.ival & val2.ival;
			else if (insn == BC_IAOR) res.ival = val.ival | val2.ival;
			else if (insn == BC_IAXOR) res.ival = val.ival ^ val2.ival;

			compStack.push(res);
			break;

		case BC_INEG:
			val = compStack.top();
			compStack.pop();
			val.ival = -val.ival;
			compStack.push(val);
			break;
		case BC_DNEG:
			val = compStack.top();
			compStack.pop();
			val.dval = -val.dval;
			compStack.push(val);
			break;

		case BC_IPRINT:
			val = compStack.top();
			compStack.pop();
			cout << val.ival;
			break;
		case BC_DPRINT:
			val = compStack.top();
			compStack.pop();
			cout << val.dval;
			break;
		case BC_SPRINT:
			val = compStack.top();
			compStack.pop();
			cout << constantById(val.sval);
			break;

		case BC_I2D:
			val = compStack.top();
			compStack.pop();
			val.dval = static_cast<double>(val.ival);
			compStack.push(val);
			break;
		case BC_D2I:
			val = compStack.top();
			compStack.pop();
			val.ival = static_cast<int64_t>(val.dval);
			compStack.push(val);
			break;
		case BC_S2I:
			val = compStack.top();
			compStack.pop();
			val.ival = val.sval;
			compStack.push(val);
			break;

		case BC_SWAP:
			val = compStack.top();
			compStack.pop();
			val2 = compStack.top();
			compStack.pop();
			compStack.push(val);
			compStack.push(val2);
			break;

		case BC_POP:
			compStack.pop();
			break;

		case BC_LOADIVAR:
		case BC_LOADDVAR:
		case BC_LOADSVAR:
			// these codes are not used by translator
			// just a stub
			varId = bc->getUInt16(ip + 1);
			ip += sizeof(uint16_t);

			val = args[varId];
			compStack.push(val);
			break;

		case BC_STOREIVAR:
		case BC_STOREDVAR:
		case BC_STORESVAR:
			varId = bc->getUInt16(ip + 1);
			ip += sizeof(uint16_t);

			val = compStack.top();
			compStack.pop();
			args.push_back(val);
			break;

		case BC_LOADCTXIVAR:
		case BC_LOADCTXDVAR:
		case BC_LOADCTXSVAR:
			ctxId = bc->getUInt16(ip + 1);
			varId = bc->getUInt16(ip + 1 + sizeof(uint16_t));
			ip += 2 * sizeof(uint16_t);

			val = contexts[ctxId].top()[varId];
			compStack.push(val);
			break;

		case BC_STORECTXIVAR:
		case BC_STORECTXDVAR:
		case BC_STORECTXSVAR:
			ctxId = bc->getUInt16(ip + 1);
			varId = bc->getUInt16(ip + 1 + sizeof(uint16_t));
			ip += 2 * sizeof(uint16_t);

			val = compStack.top();
			compStack.pop();
			contexts[ctxId].top()[varId] = val;
			break;

		case BC_ICMP:
		case BC_DCMP:
			val = compStack.top();
			compStack.pop();
			val2 = compStack.top();
			compStack.pop();
			if (insn == BC_ICMP) res.ival = val.ival < val2.ival ? -1 : val.ival > val2.ival ? 1 : 0;
			else if (insn == BC_DCMP) res.dval = val.dval < val2.dval ? -1 : val.dval > val2.dval ? 1 : 0;
			compStack.push(res);
			break;

		case BC_JA:
			ip += bc->getInt16(ip + 1);
			break;
		case BC_IFICMPE:
		case BC_IFICMPNE:
		case BC_IFICMPL:
		case BC_IFICMPLE:
		case BC_IFICMPG:
		case BC_IFICMPGE:
			val = compStack.top();
			compStack.pop();
			val2 = compStack.top();
			compStack.pop();

			if ((insn == BC_IFICMPE && val.ival == val2.ival)
			   || (insn == BC_IFICMPNE && val.ival != val2.ival)
			   || (insn == BC_IFICMPL && val.ival < val2.ival)
			   || (insn == BC_IFICMPLE && val.ival <= val2.ival)
			   || (insn == BC_IFICMPG && val.ival > val2.ival)
			   || (insn == BC_IFICMPGE && val.ival >= val2.ival)
			   || (insn == BC_JA)) {
				ip += bc->getInt16(ip + 1);
			} else {
				ip += sizeof(int16_t);
			}
			break;

		case BC_STOP:
			stopped = true;
			break;
		case BC_RETURN:
			return;

		case BC_CALL:
			ctxId = bc->getUInt16(ip + 1);
			ip += sizeof(uint16_t);

			contexts[ctxId].push(args);
			args.clear();

			executeBytecode(((BytecodeFunction*)functionById(ctxId))->bytecode());

			contexts[ctxId].pop();
			break;

		default:
			// unimplemented
			assert("false");
		}

		++ip;
	}
}
