#include "CodeInterpreter.h"

#include <string>
#include <iostream>

namespace mathvm {
	
CodeInterpreter::CodeInterpreter(void)
{
}

CodeInterpreter::~CodeInterpreter(void)
{
}

uint64_t CodeInterpreter::icmp(uint64_t op1, uint64_t op2) {
	if (op1 > op2) return 1;
	if (op1 == op2) return 0;
	return -1;
}

uint64_t CodeInterpreter::dcmp(double op1, double op2) {
	if (op1 > op2) return 1;
	if (op1 == op2) return 0;
	return -1;
}

Status* CodeInterpreter::execute(std::vector<Var*> vars) { 
	Bytecode* bytecode = _functions[0]->bytecode();
	uint32_t index = 0;
	int interrupted = 0;
	while (index < bytecode->length() && !interrupted) {
		uint8_t insn = bytecode->get(index);
		switch (insn): {
			//loading instructions 
				//for integers (64-bit int)
			case BC_ILOAD:
				_stack.push<StackVar>(StackVar(bytecode->getInt64(index + 1)));
				break;
			case BC_ILOAD0:
				_stack.push<StackVar>(StackVar(0));
				break;
			case BC_ILOAD1:
				_stack.push<StackVar>(StackVar(1));
				break;
			case BC_ILOADM1:
				_stack.push<StackVar>(StackVar(-1));
				break;
				//for double
			case BC_DLOAD:
				_stack.push<StackVar>(StackVar(bytecode->getDouble(index + 1)));
				break;
			case BC_DLOAD0:
				_stack.push<StackVar>(StackVar(0.0));
				break;
			case BC_DLOAD1:
				_stack.push<StackVar>(StackVar(1.0));
				break;
			case BC_DLOADM1:
				_stack.push<StackVar>(StackVar(-1.0));
				break;
				//for strings
			case BC_SLOAD:
				std::String str(getConstantById(bytecode->getInt16(index + 1)));
				_stack.push<StackVar>(StackVar(str.c_str()));
				break;
				
				//arithmetic operations
				//for integers
			case BC_IADD:
				uint64_t operand1 = _stack.pop<StackVar>().getInt();
				uint64_t operand2 = _stack.pop<StackVar>().getInt();
				_stack.push<StackVar>(StackVar(operand1 + operand2));
				break;
			case BC_ISUB:
				uint64_t operand1 = _stack.pop<StackVar>().getInt();
				uint64_t operand2 = _stack.pop<StackVar>().getInt();
				_stack.push<StackVar>(StackVar(operand1 - operand2));
				break;
			case BC_IMUL:
				uint64_t operand1 = _stack.pop<StackVar>().getInt();
				uint64_t operand2 = _stack.pop<StackVar>().getInt();
				_stack.push<StackVar>(StackVar(operand1 * operand2));
				break;
			case BC_IDIV:
				uint64_t operand1 = _stack.pop<StackVar>().getInt();
				uint64_t operand2 = _stack.pop<StackVar>().getInt();
				_stack.push<StackVar>(StackVar(operand1/operand2));
				break;
			case BC_INEG:
				uint64_t operand = _stack.pop<StackVar>().getInt();
				_stack.push<StackVar>(StackVar(operand*(-1)));
				break;
				//for double
			case BC_DADD:
				double operand1 = _stack.pop<StackVar>().getDouble();
				double operand2 = _stack.pop<StackVar>().getDouble();
				_stack.push<StackVar>(StackVar(operand1 + operand2));
				break;
			case BC_DSUB:
				double operand1 = _stack.pop<StackVar>().getDouble();
				double operand2 = _stack.pop<StackVar>().getDouble();
				_stack.push<StackVar>(StackVar(operand1 - operand2));
				break;
			case BC_DMUL:
				double operand1 = _stack.pop<StackVar>().getDouble();
				double operand2 = _stack.pop<StackVar>().getDouble();
				_stack.push<StackVar>(StackVar(operand1 * operand2));
				break;
			case BC_DDIV:
				double operand1 = _stack.pop<StackVar>().getDouble();
				double operand2 = _stack.pop<StackVar>().getDouble();
				_stack.push<StackVar>(StackVar(operand1/operand2));
				break;
			case BC_DNEG:
				double operand = _stack.pop<StackVar>().getDouble();
				_stack.push<StackVar>(StackVar(operand*(-1)));
				break;
				//conversion
			case BC_I2D:
				uint64_t operand = _stack.pop<StackVar>().getInt();
				_stack.push<StackVar>(StackVar((double)operand));
				break;
			case BC_D2I:
				double operand = _stack.pop<StackVar>().getDouble();
				_stack.push<StackVar>(StackVar((uint64_t)operand));
				break;
				//printing operations ????
			case BC_IPRINT:
				StackVar var = _stack.pop<StackVar>();
				std::cout << var.getInt();
				break;
			case BC_SPRINT:
				StackVar var = _stack.pop<StackVar>();
				std::cout << var.getString();
				break;
			case BC_DPRINT:
				StackVar var = _stack.pop<StackVar>();
				std::cout << var.getDouble();
				break;
				
			case BC_SWAP:
				StackVar var1 = _stack.pop<StackVar>();
				StackVar var2 = _stack.pop<StackVar>();
				_stack.push<StackVar>(var1);
				_stack.push<StackVar>(var2);
				break;
			case BC_POP:
				StackVar var = _stack.pop<StackVar>();
				break;
				//operations with variables
				//for integers
			case BC_LOADIVAR:
				uint16_t var_id = bytecode->getInt16(index + 1);
				assert(var_id < vars.size() && "Var index is out of vector bounds");
				StackVar stack_var(vars[var_id]->getIntValue());
				_stack.push<StackVar>(stack_var);
				break;
			case BC_STOREIVAR:
				uint16_t var_id = bytecode->getInt16(index + 1);
				assert(var_id < vars.size() && "Var index is out of vector bounds");
				StackVar stack_var = _stack.pop<StackVar>();
				vars[var_id]->setIntValue(stack_var.getInt());
				break;
				//for doubles
			case BC_LOADDVAR:
				uint16_t var_id = bytecode->getInt16(index + 1);
				assert(var_id < vars.size() && "Var index is out of vector bounds");
				StackVar stack_var(vars[var_id]->getDoubleValue());
				_stack.push<StackVar>(stack_var);
				break;
			case BC_STOREDVAR:
				uint16_t var_id = bytecode->getInt16(index + 1);
				assert(var_id < vars.size() && "Var index is out of vector bounds");
				StackVar stack_var = _stack.pop<StackVar>();
				vars[var_id]->setDoubleValue(stack_var.getDouble());
				break;
				//for strings
				case BC_LOADSVAR:
				uint16_t var_id = bytecode->getInt16(index + 1);
				assert(var_id < vars.size() && "Var index is out of vector bounds");
				StackVar stack_var(vars[var_id]->getStringValue());
				_stack.push<StackVar>(stack_var);
				break;
			case BC_STORESVAR:
				uint16_t var_id = bytecode->getInt16(index + 1);
				assert(var_id < vars.size() && "Var index is out of vector bounds");
				StackVar stack_var = _stack.pop<StackVar>();
				vars[var_id]->setStringValue(stack_var.getString());
				break;
				//compare operations
			case BC_ICMP:
				uint64_t operand1 = _stack.pop<StackVar>().getInt();
				uint64_t operand2 = _stack.pop<StackVar>().getInt();
				_stack.push<StackVar>(StackVar(icmp(operand1, operand2)));
				break;
			case BC_DCMP:
				double operand1 = _stack.pop<StackVar>().getDouble();
				double operand2 = _stack.pop<StackVar>().getDouble();
				_stack.push<StackVar>(StackVar(dcmp(operand1, operand2)));
				break;
				//i don`t know how these operations could be named - may be Jumps
			case BC_JA:
				uint16_t offset = bytecode->getInt16(index + 1);
				index += offset;
				break;
			case BC_IFICMPNE:
				uint64_t operand1 = _stack.pop<StackVar>().getInt();
				uint64_t operand2 = _stack.pop<StackVar>().getInt();
				if (operand1 != operand2) {
					uint16_t offset = bytecode->getInt16(index + 1);
					index += offset;
				} 
				break;
			case BC_IFICMPE:
				uint64_t operand1 = _stack.pop<StackVar>().getInt();
				uint64_t operand2 = _stack.pop<StackVar>().getInt();
				if (operand1 == operand2) {
					uint16_t offset = bytecode->getInt16(index + 1);
					index += offset;
				} 
				break;
			case BC_IFICMPG:
				uint64_t operand1 = _stack.pop<StackVar>().getInt();
				uint64_t operand2 = _stack.pop<StackVar>().getInt();
				if (operand1 > operand2) {
					uint16_t offset = bytecode->getInt16(index + 1);
					index += offset;
				} 
				break;
			case BC_IFICMPGE:
				uint64_t operand1 = _stack.pop<StackVar>().getInt();
				uint64_t operand2 = _stack.pop<StackVar>().getInt();
				if (operand1 >= operand2) {
					uint16_t offset = bytecode->getInt16(index + 1);
					index += offset;
				} 
				break;
			case BC_IFICMPL:
				uint64_t operand1 = _stack.pop<StackVar>().getInt();
				uint64_t operand2 = _stack.pop<StackVar>().getInt();
				if (operand1 < operand2) {
					uint16_t offset = bytecode->getInt16(index + 1);
					index += offset;
				} 
				break;
			case BC_IFICMPLE:
				uint64_t operand1 = _stack.pop<StackVar>().getInt();
				uint64_t operand2 = _stack.pop<StackVar>().getInt();
				if (operand1 <= operand2) {
					uint16_t offset = bytecode->getInt16(index + 1);
					index += offset;
				} 
				break;
			case BC_DUMP:
				StackVar tos = _stack.get<StackVar>();
				StackVarType type = tos.getType();
				if (type == SV_INT) std::cout << tos.getInt();
				if (type == SV_DOUBLE) std::cout << tos.getDouble();
				if (type == SV_STRING) std::cout << tos.getString();
				break;
			case BC_STOP:
				interrupted = 1;
				break;
			case BC_CALL:
				break;
			case BC_RETURN:
				break;
			case BC_BREAK:
				break;
			default:
				break;
		}
		ip += instruction_size(insn);	
			
		}
	}
}

