#include "ExecutableCode.h"
#include "ast.h"
#include <stack>
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

	std::stack<unit> stack;
	double dVar0, dVar1, dVar2, dVar3;
	int64_t iVar0, iVar1, iVar2, iVar3;
	string sVar0, sVar1, sVar2, sVar3;

	std::map<uint16_t, int64_t> intVarMap;
	std::map<uint16_t, double>  doubleVarMap;
	std::map<uint16_t, string>  strVarMap;

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
				//std::cerr<<"BC_DLOAD"<<std::endl;
				stack.push(u);
				break;
			case BC_ILOAD:
				u.intVal = bytecode->getInt64(insnIndex + 1);
				//std::cerr<<"BC_ILOAD u.intVal = "<<u.intVal<<std::endl;
				stack.push(u);
				break;
			case BC_SLOAD:
				u.idVal = bytecode->getInt16(insnIndex + 1);
				stack.push(u);
				break;
			case BC_DLOAD0:
				u.doubleVal = 0.0;
				stack.push(u);
				break;
			case BC_ILOAD0:
				u.intVal = 0;
				stack.push(u);
				//std::cerr<<"BC_ILOAD0"<<std::endl;
				break;
			case BC_SLOAD0:
				u.idVal = makeStringConstant("");
				stack.push(u);
				break;
			case BC_DLOAD1:
				u.doubleVal = 1.0;
				stack.push(u);
				break;
			case BC_ILOAD1:
				u.intVal = 1;
				stack.push(u);
				//std::cerr<<"BC_ILOAD1"<<std::endl;
				break;
			case BC_DLOADM1:
				u.doubleVal = -1.0;
				stack.push(u);
				break;
			case BC_ILOADM1:
				u.intVal = -1;
				stack.push(u);
				break;
			case BC_DADD:
				dUpper = stack.top().doubleVal; stack.pop();
				dLower = stack.top().doubleVal; stack.pop();
				u.doubleVal = dUpper + dLower;
				//std::cerr<<"BC_dADD dUpper="<<dUpper<<" dLower="<<dLower<<" u="<<u.doubleVal<<std::endl;
				stack.push(u);
				break;
			case BC_IADD:
				iUpper = stack.top().intVal; stack.pop();
				iLower = stack.top().intVal; stack.pop();
				u.intVal = iUpper + iLower;
				//std::cerr<<"BC_IADD iUpper="<<iUpper<<" iLower="<<iLower<<" u="<<u.intVal<<std::endl;
				stack.push(u);
				break;
			case BC_DSUB:
				dUpper = stack.top().doubleVal; stack.pop();
				dLower = stack.top().doubleVal; stack.pop();
				u.doubleVal = dUpper - dLower;
				stack.push(u);
				break;
			case BC_ISUB:
				iUpper = stack.top().intVal; stack.pop();
				iLower = stack.top().intVal; stack.pop();
				u.intVal = iUpper - iLower;
				stack.push(u);
				break;
			case BC_DMUL:
				dUpper = stack.top().doubleVal; stack.pop();
				dLower = stack.top().doubleVal; stack.pop();
				u.doubleVal = dUpper * dLower;
				stack.push(u);
				break;
			case BC_IMUL:
				iUpper = stack.top().intVal; stack.pop();
				iLower = stack.top().intVal; stack.pop();
				u.intVal = iUpper * iLower;
				stack.push(u);
				break;
			case BC_DDIV:
				dUpper = stack.top().doubleVal; stack.pop();
				dLower = stack.top().doubleVal; stack.pop();
				u.doubleVal = dUpper / dLower;
				stack.push(u);
				break;
			case BC_IDIV:
				iUpper = stack.top().intVal; stack.pop();
				iLower = stack.top().intVal; stack.pop();
				u.intVal = iUpper / iLower;
				stack.push(u);
				break;
			case BC_IMOD:
				iUpper = stack.top().intVal; stack.pop();
				iLower = stack.top().intVal; stack.pop();
				u.intVal = iUpper % iLower;
				stack.push(u);
				break;
			case BC_DNEG:
				d = stack.top().doubleVal; stack.pop();
				u.doubleVal = -d;
				stack.push(u);
				break;
			case BC_INEG:
				i = stack.top().intVal; stack.pop();
				u.intVal = -i;
				stack.push(u);
				break;
			case BC_IPRINT:
				i = stack.top().intVal; stack.pop();
				//std::cerr<<"BC_IPRINT"<<std::endl;
				std::cout<<i;
				break;
			case BC_DPRINT:
				d = stack.top().doubleVal; stack.pop();
				//std::cerr<<"BC_DPRINT!!!"<<std::endl;
				std::cout<<d;
				break;
			case BC_SPRINT:
				str = constantById(stack.top().idVal); stack.pop();
				std::cout<<str;
				break;
			case BC_I2D:

				break;
			case BC_D2I:

				break;
			case BC_S2I:

				break;
			case BC_SWAP:
				u1 = stack.top(); stack.pop();
				u2 = stack.top(); stack.pop();
				stack.push(u1);
				stack.push(u2);
				break;
			case BC_POP:
				stack.pop();
				//std::cerr<<"BC_POP"<<std::endl;
				break;
			case BC_LOADDVAR0:
				u.doubleVal = dVar0;
				stack.push(u);
				break;
			case BC_LOADDVAR1:
				u.doubleVal = dVar1;
				stack.push(u);
				break;
			case BC_LOADDVAR2:
				u.doubleVal = dVar2;
				stack.push(u);
				break;
			case BC_LOADDVAR3:
				u.doubleVal = dVar3;
				stack.push(u);
				break;
			case BC_LOADIVAR0:
				u.intVal = iVar0;
				stack.push(u);
				break;
			case BC_LOADIVAR1:
				u.intVal = iVar1;
				stack.push(u);
				break;
			case BC_LOADIVAR2:
				u.intVal = iVar2;
				stack.push(u);
				break;
			case BC_LOADIVAR3:
				u.intVal = iVar3;
				stack.push(u);
				break;
			case BC_LOADSVAR0:
				u.idVal = makeStringConstant(sVar0);
				stack.push(u);
				break;
			case BC_LOADSVAR1:
				u.idVal = makeStringConstant(sVar1);
				stack.push(u);
				break;
			case BC_LOADSVAR2:
				u.idVal = makeStringConstant(sVar2);
				stack.push(u);
				break;
			case BC_LOADSVAR3:
				u.idVal = makeStringConstant(sVar3);
				stack.push(u);
				break;
			case BC_STOREDVAR0:
				dVar0 = stack.top().doubleVal; stack.pop();
				break;
			case BC_STOREDVAR1:
				dVar1 = stack.top().doubleVal; stack.pop();
				break;
			case BC_STOREDVAR2:
				dVar2 = stack.top().doubleVal; stack.pop();
				break;
			case BC_STOREDVAR3:
				dVar3 = stack.top().doubleVal; stack.pop();
				break;
			case BC_STOREIVAR0:
				iVar0 = stack.top().intVal; stack.pop();
				break;
			case BC_STOREIVAR1:
				iVar1 = stack.top().intVal; stack.pop();
				break;
			case BC_STOREIVAR2:
				iVar2 = stack.top().intVal; stack.pop();
				break;
			case BC_STOREIVAR3:
				iVar3 = stack.top().intVal; stack.pop();
				break;
			case BC_STORESVAR0:
				sVar0 = constantById(stack.top().idVal); stack.pop();
				break;
			case BC_STORESVAR1:
				sVar1 = constantById(stack.top().idVal); stack.pop();
				break;
			case BC_STORESVAR2:
				sVar2 = constantById(stack.top().idVal); stack.pop();
				break;
			case BC_STORESVAR3:
				sVar3 = constantById(stack.top().idVal); stack.pop();
				break;
			case BC_LOADDVAR:
				u.doubleVal = doubleVarMap[bytecode->getInt16(insnIndex + 1)];
				//std::cerr<<"BC_LOADDVAR"<<std::endl;
				stack.push(u);
				break;
			case BC_LOADIVAR:
				u.intVal = intVarMap[bytecode->getUInt16(insnIndex + 1)];
				//std::cerr<<"BC_LOADIVAR u.intVal = "<<u.intVal<<std::endl;
				stack.push(u);
				break;
			case BC_LOADSVAR:
				u.idVal = makeStringConstant(strVarMap[bytecode->getUInt16(insnIndex + 1)]);
				stack.push(u);
				break;
			case BC_STOREDVAR:
				d = stack.top().doubleVal; stack.pop();
				doubleVarMap[bytecode->getInt16(insnIndex + 1)] = d;
				break;
			case BC_STOREIVAR:
				i = stack.top().intVal; stack.pop();
				intVarMap[bytecode->getUInt16(insnIndex + 1)] = i;
				//std::cerr<<"BC_STOREIVAR i = "<<i<<std::endl;
				break;
			case BC_STORESVAR:
				str = constantById(stack.top().idVal); stack.pop();
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
				u = stack.top();
				dUpper = u.doubleVal; stack.pop();
				dLower = stack.top().doubleVal; stack.push(u);
				if(dUpper > dLower) u.doubleVal = 1;
				if(dUpper == dLower) u.doubleVal = 0;
				if(dUpper < dLower) u.doubleVal = -1;
				stack.push(u);
				break;
			case BC_ICMP:
				u = stack.top();
				iUpper = u.intVal; stack.pop();
				iLower = stack.top().intVal; stack.push(u);
				if(iUpper > iLower) u.intVal = 1;
				if(iUpper == iLower) u.intVal = 0;
				if(iUpper < iLower) u.intVal = -1;
				stack.push(u);
				break;
			case BC_JA:
				insnIndex += bytecode->getInt16(insnIndex + 1) + 1;
				//std::cerr<<"BC_JA insnIndex = "<<insnIndex<<std::endl;
				continue;
				break;
			case BC_IFICMPNE:
				u = stack.top();
				iUpper = u.intVal; stack.pop();
				iLower = stack.top().intVal; stack.push(u);
				//std::cerr<<"BC_IFICMPNE iUpper = "<<iUpper<<" iLower = "<<iLower<<std::endl;
				if(iUpper != iLower) {
					insnIndex += bytecode->getInt16(insnIndex + 1) + 1;
					continue;
				}
				break;
			case BC_IFICMPE:
				u = stack.top();
				iUpper = u.intVal; stack.pop();
				iLower = stack.top().intVal; stack.push(u);
				if(iUpper == iLower) {
					insnIndex += bytecode->getInt16(insnIndex + 1) + 1;
					continue;
				}
				break;
			case BC_IFICMPG:
				u = stack.top();
				iUpper = u.intVal; stack.pop();
				iLower = stack.top().intVal; stack.push(u);
				//std::cerr<<"BC_IFICMPG u = "<<iUpper<<" l = "<<iLower<<std::endl;
				if(iUpper > iLower) {
					insnIndex += bytecode->getInt16(insnIndex + 1) + 1;
					//std::cerr<<"!!!! insnIndex = "<<insnIndex<<std::endl;
					continue;
				}
				break;
			case BC_IFICMPGE:
				u = stack.top();
				iUpper = u.intVal; stack.pop();
				iLower = stack.top().intVal; stack.push(u);
				if(iUpper >= iLower) {
					insnIndex += bytecode->getInt16(insnIndex + 1) + 1;
					continue;
				}
				break;
			case BC_IFICMPL:
				u = stack.top();
				iUpper = u.intVal; stack.pop();
				iLower = stack.top().intVal; stack.push(u);
				if(iUpper < iLower) {
					insnIndex += bytecode->getInt16(insnIndex + 1) + 1;
					continue;
				}
				break;
			case BC_IFICMPLE:
				u = stack.top();
				iUpper = u.intVal; stack.pop();
				iLower = stack.top().intVal; stack.push(u);
				//std::cerr<<"BC_IFICMPLE iUpper="<<iUpper<<" iLower="<<iLower<<std::endl;
				if(iUpper <= iLower) {
					insnIndex += bytecode->getInt16(insnIndex + 1) + 1;
					continue;
				}
				break;
			case BC_DUMP:
				//std::cout<<stack.top().intVal<<std::endl;
				break;
			case BC_STOP:
				return new Status();
				break;
			case BC_CALL:

				break;
			case BC_CALLNATIVE:

				break;
			case BC_RETURN:

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
