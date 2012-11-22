#include "CodeImpl.h"
#include <stack>

namespace mathvm {

Status* CodeImpl::execute(vector<Var*>&) {
	BytecodeFunction* fun = (BytecodeFunction*) functionById(0);
	Bytecode* bytecode = fun->bytecode();

	std::stack<Var> stack;
	uint32_t ip = 0;
	while (ip < bytecode->length()) {
		Instruction instruction = bytecode->getInsn(ip++);
		switch (instruction) {
			case BC_DPRINT: {
				double val = stack.top().getDoubleValue();
				stack.pop();
				std::cout << val << std::endl;;
				break;
			}
			case BC_IPRINT: {
				int64_t val = stack.top().getIntValue();
				stack.pop();
				std::cout << val << std::endl;;
				break;
			}
			case BC_SPRINT: { 
				uint16_t id = stack.top().getIntValue();
				stack.pop();
				std::cout << constantById(id) << std::endl;;
				break;
			}
			case BC_DLOAD: {
				double val = bytecode->getDouble(ip);
				ip += 8;
				Var var(VT_DOUBLE, "");
				var.setDoubleValue(val);
				stack.push(var);
				break;
			}
			case BC_ILOAD: {
				int64_t val = bytecode->getInt64(ip);
				ip += 8;
				Var var(VT_INT, "");
				var.setIntValue(val);
				stack.push(var);
				break;
			}
			case BC_SLOAD: {
				uint16_t val= bytecode->getUInt16(ip);
				ip += 2;
				Var id(VT_INT, "");
				id.setIntValue(val);
				stack.push(id);
				break;
			}
			case BC_DLOAD0: { 
				Var var(VT_DOUBLE, "");
				var.setDoubleValue(0);
				stack.push(var);
				break;
			}
			case BC_ILOAD0: { 
				Var var(VT_INT, "");
				var.setIntValue(0);
				stack.push(var);
				break;
			}
			case BC_SLOAD0: { 
				Var var(VT_INT, "");
				var.setIntValue(makeStringConstant(""));
				stack.push(var);
				break;
			}
			case BC_DLOAD1: { 
				Var var(VT_DOUBLE, "");
				var.setDoubleValue(1);
				stack.push(var);
				break;
			}
			case BC_ILOAD1: {
				Var var(VT_INT, "");
				var.setIntValue(1);
				stack.push(var);
				break;
			}
			case BC_DLOADM1: {
				Var var(VT_DOUBLE, "");
				var.setDoubleValue(-1);
				stack.push(var);
				break;
			}
			case BC_ILOADM1: {
				Var var(VT_INT, "");
				var.setIntValue(-1);
				stack.push(var);
				break;
			}
			default: ; 
		}
	}

	return 0;
}

}
