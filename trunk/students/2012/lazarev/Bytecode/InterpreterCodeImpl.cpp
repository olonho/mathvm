#include "InterpreterCodeImpl.h"
#include "mathvm.h"

namespace mathvm {
	
	
	static int commandLen[] = {
		#define ENUM_ELEM(b, d, l) l,
			FOR_BYTECODES(ENUM_ELEM)
		#undef ENUM_ELEM
		1
	};
}

namespace mathvm {


Status* InterpreterCodeImpl::execute(vector<Var*>& vars) {
	disassemble();
	BytecodeFunction* f = (BytecodeFunction*)functionById(0);
	runBytecode(f->bytecode());
	return new Status();
}

void InterpreterCodeImpl::runBytecode(Bytecode *bytecode) {
	size_t bci = 0;
	while (bci < (bytecode -> length())) {
		Instruction insn = bytecode -> getInsn(bci);
		
		switch (insn) {
			case BC_DLOAD:
				push(getDoubleVar(bytecode -> getDouble(bci + 1)));
				break;
			case BC_ILOAD:
				push(getIntVar(bytecode -> getInt64(bci + 1)));
				break;
			case BC_SLOAD:
				push(getStringVar(constantById(bytecode -> getUInt16(bci + 1)).c_str()));
				break;

			case BC_DLOAD0:
				push(getDoubleVar(0.0));
				break;
			case BC_ILOAD0:
				push(getIntVar(0L));
				break;
			case BC_SLOAD0:
				push(getStringVar(""));
				break;

			case BC_DLOAD1:
				push(getDoubleVar(1.0));
				break;
			case BC_ILOAD1:
				push(getIntVar(1L));
				break;

			case BC_DLOADM1:
				push(getDoubleVar(-1.0));
				break;
			case BC_ILOADM1:
				push(getIntVar(-1L));
				break;

			case BC_DADD: {
				double first = popDouble();
				double second = popDouble();
				push(getDoubleVar(first + second));
				break;
			}
			case BC_IADD: {
				int64_t first = popInt();
				int64_t second = popInt();
				push(getIntVar(first + second));
				break;
			}

			case BC_DSUB: {
				double first = popDouble();
				double second = popDouble();
				push(getDoubleVar(first - second));
				break;
			}
			case BC_ISUB: {
				int64_t first = popInt();
				int64_t second = popInt();
				push(getIntVar(first - second));
				break;
			}

			case BC_DMUL: {
				double first = popDouble();
				double second = popDouble();
				push(getDoubleVar(first * second));
				break;
			}
			case BC_IMUL: {
				int64_t first = popInt();
				int64_t second = popInt();
				push(getIntVar(first * second));
				break;
			}

			case BC_DDIV: {
				double first = popDouble();
				double second = popDouble();
				push(getDoubleVar(first / second));
				break;
			}
			case BC_IDIV: {
				int64_t first = popInt();
				int64_t second = popInt();
				push(getIntVar(first / second));
				break;
			}
			case BC_IMOD: {
				int64_t first = popInt();
				int64_t second = popInt();
				push(getIntVar(first % second));
				break;
			}

			case BC_DNEG: {
				double val = popDouble();
				push(getDoubleVar(-val));
				break;
			}
			case BC_INEG: {
				int64_t val = popInt();
				push(getIntVar(-val));
				break;
			}

			case BC_DPRINT: {
				double val = popDouble();
				cout << val;
				break;
			}
			case BC_IPRINT: {
				int64_t val = popInt();
				cout << val;
				break;
			}
			case BC_SPRINT: {
				const char* val = popString();
				cout << val;
				break;
			}

			case BC_I2D: {
				int64_t val = popInt();
				push(getDoubleVar((double)val));
				break;
			}
			case BC_D2I: {
				double val = popDouble();
				push(getIntVar((int64_t )val));
				break;
			}

			case BC_POP:
				programStack.pop();
				break;


			case BC_DCMP: {
				double first = popDouble();
				double second = popDouble();
				if (first == second) {
					push(getIntVar(0));
				} else if (first > second) {
					push(getIntVar(1));
				} else {
					push(getIntVar(-1));
				}
				break;
			}
			case BC_ICMP: {
				int64_t first = popInt();
				int64_t second = popInt();
				if (first == second) {
					push(getIntVar(0));
				} else if (first > second) {
					push(getIntVar(1));
				} else {
					push(getIntVar(-1));
				}
				break;
			}

			case BC_JA:
				bci += (bytecode -> getInt16(bci + 1));
				break;
			case BC_IFICMPNE: {
				int64_t first = popInt();
				int64_t second = popInt();
				push(getIntVar(second));
				push(getIntVar(first));
				if (first != second) {
					bci += bytecode -> getInt16(bci + 1);
				}
				break;
			}
			case BC_IFICMPE: {
				int64_t first = popInt();
				int64_t second = popInt();
				push(getIntVar(second));
				push(getIntVar(first));
				if (first == second) {
					bci += bytecode -> getInt16(bci + 1);
				}
				break;
			}
			case BC_IFICMPG: {
				int64_t first = popInt();
				int64_t second = popInt();
				push(getIntVar(second));
				push(getIntVar(first));
				if (first > second) {
					bci += bytecode -> getInt16(bci + 1);
				}
				break;
			}
			case BC_IFICMPGE: {
				int64_t first = popInt();
				int64_t second = popInt();
				push(getIntVar(second));
				push(getIntVar(first));
				if (first >= second) {
					bci += bytecode -> getInt16(bci + 1);
				}
				break;
			}
			case BC_IFICMPL: {
				int64_t first = popInt();
				int64_t second = popInt();
				push(getIntVar(second));
				push(getIntVar(first));
				if (first < second) {
					bci += bytecode -> getInt16(bci + 1);
				}
				break;
			}
			case BC_IFICMPLE: {
				int64_t first = popInt();
				int64_t second = popInt();
				push(getIntVar(second));
				push(getIntVar(first));
				if (first <= second) {
					bci += bytecode -> getInt16(bci + 1);
				}
				break;
			}
				
				
			case BC_CALL: {
				int id = bytecode->getInt16(bci + 1);
				BytecodeFunction *f = (BytecodeFunction*)functionById(id);
				
				bytecodes.push(bytecode);
				bcis.push(bci + commandLen[insn]);
				
				bytecode = f->bytecode();
				bci = 0;
				continue;
			}
			case BC_RETURN: {
				
				bytecode = bytecodes.top();
				bci = bcis.top();
				
				bytecodes.pop();
				bcis.pop();
				
				continue;
			}
				
			case BC_LOADDVAR:
			case BC_LOADIVAR:
			case BC_LOADSVAR: {
				int id = bytecode->getInt16(bci + 1);
				push(mem[id]);
				break;
			}
				
			case BC_LOADDVAR0:
			case BC_LOADIVAR0:
			case BC_LOADSVAR0: {
				push(mem[0]);
				break;
			}
			
			case BC_LOADDVAR1:
			case BC_LOADIVAR1:
			case BC_LOADSVAR1: {
				push(mem[1]);
				break;
			}

			case BC_LOADDVAR2:
			case BC_LOADIVAR2:
			case BC_LOADSVAR2: {
				push(mem[2]);
				break;
			}
			
			case BC_LOADDVAR3:
			case BC_LOADIVAR3:
			case BC_LOADSVAR3: {
				push(mem[3]);
				break;
			}
				
			case BC_STOREDVAR:
			case BC_STOREIVAR:
			case BC_STORESVAR: {
				int id = bytecode->getInt16(bci + 1);
				mem[id] = pop();
				break;
			}
				
			case BC_STOREDVAR0:
			case BC_STOREIVAR0:
			case BC_STORESVAR0: {
				mem[0] = pop();
				break;
			}
				
			case BC_STOREDVAR1:
			case BC_STOREIVAR1:
			case BC_STORESVAR1: {
				mem[1] = pop();
				break;
			}
				
			case BC_STOREDVAR2:
			case BC_STOREIVAR2:
			case BC_STORESVAR2: {
				mem[2] = pop();
				break;
			}
				
			case BC_STOREDVAR3:
			case BC_STOREIVAR3:
			case BC_STORESVAR3: {
				mem[3] = pop();
				break;
			}
				
			case BC_SWAP: {
				Var* val1 = pop();
				Var* val2 = pop();
				push(val1);
				push(val2);
				break;
			}
				

			default:
				throw std::exception();
		}

		bci += commandLen[insn];
	}
}

Var *InterpreterCodeImpl::getDoubleVar(double val)
{
	Var* var = new Var(VT_DOUBLE, "");
	var->setDoubleValue(val);
	return var;
}

Var *InterpreterCodeImpl::getIntVar(int64_t val)
{
	Var* var = new Var(VT_INT, "");
	var->setIntValue(val);
	return var;
}

Var *InterpreterCodeImpl::getStringVar(const char *val)
{
	Var* var = new Var(VT_STRING, "");
	var->setStringValue(val);
	return var;
}


double InterpreterCodeImpl::popDouble() {
	assert(!programStack.empty());
	Var* var = programStack.top();
	programStack.pop();
	return var->getDoubleValue();
}

int64_t InterpreterCodeImpl::popInt() {
	assert(!programStack.empty());
	Var* var = programStack.top();
	programStack.pop();
	return var->getIntValue();
}

const char *InterpreterCodeImpl::popString() {
	assert(!programStack.empty());
	Var* var = programStack.top();
	programStack.pop();
	return var->getStringValue();
}

void InterpreterCodeImpl::push(Var *val) {
	programStack.push(val);
}

Var* InterpreterCodeImpl::pop() {
	assert(!programStack.empty());
	Var* res = programStack.top();
	programStack.pop();
	return res;
}


}
