#include "ExecutableCode.h"
#include "ast.h"
#include <string>

namespace mathvm {

ExecutableCode::ExecutableCode() {

}

ExecutableCode::~ExecutableCode() {

}

static size_t getInsnLength(Instruction insn) {
	static const struct
    {
		const char* name;
		Instruction insn;
		size_t length;
    } names[] = {
		#define BC_NAME(b, d, l) {#b, BC_##b, l},
    		FOR_BYTECODES(BC_NAME)
        };

    return names[insn].length;
}

Status* ExecutableCode::execute(std::vector<Var*>& vars) {
	Bytecode *bytecode = ((BytecodeFunction *) functionByName(AstFunction::top_name))->bytecode();
	return execute(bytecode);
}

Status* ExecutableCode::execute(Bytecode *bytecode) {


	double dVar0, dVar1, dVar2, dVar3;
	int64_t iVar0, iVar1, iVar2, iVar3;
	string sVar0, sVar1, sVar2, sVar3;

	std::map<uint16_t, int64_t> intVarMap;
	std::map<uint16_t, double>  doubleVarMap;
	std::map<uint16_t, string>  strVarMap;

	std::pair<uint16_t, uint16_t> varIds;


	uint32_t bytecodeLength = bytecode->length();

	for(uint32_t insnIndex = 0; insnIndex < bytecodeLength;){

		Instruction insn = bytecode->getInsn(insnIndex);

		unit u, u1, u2;
		double dUpper, dLower, d;
		int64_t iUpper, iLower, i;
		string str;

		switch(insn){
			case BC_INVALID:
				std::cerr<<"Error: invalid instruction"<<std::endl;
				return new Status("Invalid instruction");
				break;
			case BC_DLOAD:
				u.doubleVal = bytecode->getDouble(insnIndex + 1);
				mStack.push(u);
				break;
			case BC_ILOAD:
				u.intVal = bytecode->getInt64(insnIndex + 1);
				mStack.push(u);
				break;
			case BC_SLOAD:
				u.strValId = bytecode->getInt16(insnIndex + 1);
				mStack.push(u);
				break;
			case BC_DLOAD0:
				u.doubleVal = 0.0;
				mStack.push(u);
				break;
			case BC_ILOAD0:
				u.intVal = 0;
				mStack.push(u);
				break;
			case BC_SLOAD0:
				u.strValId = makeStringConstant("");
				mStack.push(u);
				break;
			case BC_DLOAD1:
				u.doubleVal = 1.0;
				mStack.push(u);
				break;
			case BC_ILOAD1:
				u.intVal = 1;
				mStack.push(u);
				break;
			case BC_DLOADM1:
				u.doubleVal = -1.0;
				mStack.push(u);
				break;
			case BC_ILOADM1:
				u.intVal = -1;
				mStack.push(u);
				break;
			case BC_DADD:
				dUpper = mStack.top().doubleVal; mStack.pop();
				dLower = mStack.top().doubleVal; mStack.pop();
				u.doubleVal = dUpper + dLower;
				mStack.push(u);
				break;
			case BC_IADD:
				iUpper = mStack.top().intVal; mStack.pop();
				iLower = mStack.top().intVal; mStack.pop();
				u.intVal = iUpper + iLower;
				mStack.push(u);
				break;
			case BC_DSUB:
				dUpper = mStack.top().doubleVal; mStack.pop();
				dLower = mStack.top().doubleVal; mStack.pop();
				u.doubleVal = dUpper - dLower;
				mStack.push(u);
				break;
			case BC_ISUB:
				iUpper = mStack.top().intVal; mStack.pop();
				iLower = mStack.top().intVal; mStack.pop();
				u.intVal = iUpper - iLower;
				mStack.push(u);
				break;
			case BC_DMUL:
				dUpper = mStack.top().doubleVal; mStack.pop();
				dLower = mStack.top().doubleVal; mStack.pop();
				u.doubleVal = dUpper * dLower;
				mStack.push(u);
				break;
			case BC_IMUL:
				iUpper = mStack.top().intVal; mStack.pop();
				iLower = mStack.top().intVal; mStack.pop();
				u.intVal = iUpper * iLower;
				mStack.push(u);
				break;
			case BC_DDIV:
				dUpper = mStack.top().doubleVal; mStack.pop();
				dLower = mStack.top().doubleVal; mStack.pop();
				u.doubleVal = dUpper / dLower;
				mStack.push(u);
				break;
			case BC_IDIV:
				iUpper = mStack.top().intVal; mStack.pop();
				iLower = mStack.top().intVal; mStack.pop();
				u.intVal = iUpper / iLower;
				mStack.push(u);
				break;
			case BC_IMOD:
				iUpper = mStack.top().intVal; mStack.pop();
				iLower = mStack.top().intVal; mStack.pop();
				u.intVal = iUpper % iLower;
				mStack.push(u);
				break;
			case BC_DNEG:
				d = mStack.top().doubleVal; mStack.pop();
				u.doubleVal = -d;
				mStack.push(u);
				break;
			case BC_INEG:
				i = mStack.top().intVal; mStack.pop();
				u.intVal = -i;
				mStack.push(u);
				break;
			case BC_IPRINT:
				i = mStack.top().intVal; mStack.pop();
				std::cout<<i;
				break;
			case BC_DPRINT:
				d = mStack.top().doubleVal; mStack.pop();
				std::cout<<d;
				break;
			case BC_SPRINT:
				str = constantById(mStack.top().strValId); mStack.pop();
				std::cout<<str;
				break;
			case BC_I2D:

				break;
			case BC_D2I:

				break;
			case BC_S2I:

				break;
			case BC_SWAP:
				u1 = mStack.top(); mStack.pop();
				u2 = mStack.top(); mStack.pop();
				mStack.push(u1);
				mStack.push(u2);
				break;
			case BC_POP:
				mStack.pop();
				break;
			case BC_LOADDVAR0:
				u.doubleVal = dVar0;
				mStack.push(u);
				break;
			case BC_LOADDVAR1:
				u.doubleVal = dVar1;
				mStack.push(u);
				break;
			case BC_LOADDVAR2:
				u.doubleVal = dVar2;
				mStack.push(u);
				break;
			case BC_LOADDVAR3:
				u.doubleVal = dVar3;
				mStack.push(u);
				break;
			case BC_LOADIVAR0:
				u.intVal = iVar0;
				mStack.push(u);
				break;
			case BC_LOADIVAR1:
				u.intVal = iVar1;
				mStack.push(u);
				break;
			case BC_LOADIVAR2:
				u.intVal = iVar2;
				mStack.push(u);
				break;
			case BC_LOADIVAR3:
				u.intVal = iVar3;
				mStack.push(u);
				break;
			case BC_LOADSVAR0:
				u.strValId = makeStringConstant(sVar0);
				mStack.push(u);
				break;
			case BC_LOADSVAR1:
				u.strValId = makeStringConstant(sVar1);
				mStack.push(u);
				break;
			case BC_LOADSVAR2:
				u.strValId = makeStringConstant(sVar2);
				mStack.push(u);
				break;
			case BC_LOADSVAR3:
				u.strValId = makeStringConstant(sVar3);
				mStack.push(u);
				break;
			case BC_STOREDVAR0:
				dVar0 = mStack.top().doubleVal; mStack.pop();
				break;
			case BC_STOREDVAR1:
				dVar1 = mStack.top().doubleVal; mStack.pop();
				break;
			case BC_STOREDVAR2:
				dVar2 = mStack.top().doubleVal; mStack.pop();
				break;
			case BC_STOREDVAR3:
				dVar3 = mStack.top().doubleVal; mStack.pop();
				break;
			case BC_STOREIVAR0:
				iVar0 = mStack.top().intVal; mStack.pop();
				break;
			case BC_STOREIVAR1:
				iVar1 = mStack.top().intVal; mStack.pop();
				break;
			case BC_STOREIVAR2:
				iVar2 = mStack.top().intVal; mStack.pop();
				break;
			case BC_STOREIVAR3:
				iVar3 = mStack.top().intVal; mStack.pop();
				break;
			case BC_STORESVAR0:
				sVar0 = constantById(mStack.top().strValId); mStack.pop();
				break;
			case BC_STORESVAR1:
				sVar1 = constantById(mStack.top().strValId); mStack.pop();
				break;
			case BC_STORESVAR2:
				sVar2 = constantById(mStack.top().strValId); mStack.pop();
				break;
			case BC_STORESVAR3:
				sVar3 = constantById(mStack.top().strValId); mStack.pop();
				break;
			case BC_LOADDVAR:
				u.doubleVal = doubleVarMap[bytecode->getInt16(insnIndex + 1)];
				mStack.push(u);
				break;
			case BC_LOADIVAR:
				u.intVal = intVarMap[bytecode->getUInt16(insnIndex + 1)];
				mStack.push(u);
				break;
			case BC_LOADSVAR:
				u.strValId = makeStringConstant(strVarMap[bytecode->getUInt16(insnIndex + 1)]);
				mStack.push(u);
				break;
			case BC_STOREDVAR:
				d = mStack.top().doubleVal; mStack.pop();
				doubleVarMap[bytecode->getInt16(insnIndex + 1)] = d;
				break;
			case BC_STOREIVAR:
				i = mStack.top().intVal; mStack.pop();
				intVarMap[bytecode->getUInt16(insnIndex + 1)] = i;
				break;
			case BC_STORESVAR:
				str = constantById(mStack.top().strValId); mStack.pop();
				strVarMap[bytecode->getUInt16(insnIndex + 1)] = str;
				break;
			case BC_LOADCTXDVAR:
				break;
			case BC_LOADCTXIVAR:
				break;
			case BC_LOADCTXSVAR:
				break;
			case BC_STORECTXDVAR:
				break;
			case BC_STORECTXIVAR:
				break;
			case BC_STORECTXSVAR:
				break;
			case BC_DCMP:
				u = mStack.top();
				dUpper = u.doubleVal; mStack.pop();
				dLower = mStack.top().doubleVal; mStack.push(u);
				if(dUpper > dLower) u.doubleVal = 1;
				if(dUpper == dLower) u.doubleVal = 0;
				if(dUpper < dLower) u.doubleVal = -1;
				mStack.push(u);
				break;
			case BC_ICMP:
				u = mStack.top();
				iUpper = u.intVal; mStack.pop();
				iLower = mStack.top().intVal; mStack.push(u);
				if(iUpper > iLower) u.intVal = 1;
				if(iUpper == iLower) u.intVal = 0;
				if(iUpper < iLower) u.intVal = -1;
				mStack.push(u);
				break;
			case BC_JA:
				insnIndex += bytecode->getInt16(insnIndex + 1) + 1;
				continue;
				break;
			case BC_IFICMPNE:
				u = mStack.top();
				iUpper = u.intVal; mStack.pop();
				iLower = mStack.top().intVal; mStack.push(u);
				if(iUpper != iLower) {
					insnIndex += bytecode->getInt16(insnIndex + 1) + 1;
					continue;
				}
				break;
			case BC_IFICMPE:
				u = mStack.top();
				iUpper = u.intVal; mStack.pop();
				iLower = mStack.top().intVal; mStack.push(u);
				if(iUpper == iLower) {
					insnIndex += bytecode->getInt16(insnIndex + 1) + 1;
					continue;
				}
				break;
			case BC_IFICMPG:
				u = mStack.top();
				iUpper = u.intVal; mStack.pop();
				iLower = mStack.top().intVal; mStack.push(u);
				if(iUpper > iLower) {
					insnIndex += bytecode->getInt16(insnIndex + 1) + 1;
					continue;
				}
				break;
			case BC_IFICMPGE:
				u = mStack.top();
				iUpper = u.intVal; mStack.pop();
				iLower = mStack.top().intVal; mStack.push(u);
				if(iUpper >= iLower) {
					insnIndex += bytecode->getInt16(insnIndex + 1) + 1;
					continue;
				}
				break;
			case BC_IFICMPL:
				u = mStack.top();
				iUpper = u.intVal; mStack.pop();
				iLower = mStack.top().intVal; mStack.push(u);
				if(iUpper < iLower) {
					insnIndex += bytecode->getInt16(insnIndex + 1) + 1;
					continue;
				}
				break;
			case BC_IFICMPLE:
				u = mStack.top();
				iUpper = u.intVal; mStack.pop();
				iLower = mStack.top().intVal; mStack.push(u);
				if(iUpper <= iLower) {
					insnIndex += bytecode->getInt16(insnIndex + 1) + 1;
					continue;
				}
				break;
			case BC_DUMP:
				break;
			case BC_STOP:
				break;
			case BC_CALL:
				execute(((BytecodeFunction*) functionById(bytecode->getUInt16(insnIndex + 1)))->bytecode());
				break;
			case BC_CALLNATIVE:

				break;
			case BC_RETURN:
				return new Status();
				break;
			case BC_BREAK:

				break;
			default:
				break;
		}
		insnIndex += getInsnLength(insn);
	}
	return new Status();
}

}
