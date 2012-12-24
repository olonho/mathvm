#include <iostream>
#include <vector>
#include <map>

using namespace mathvm;
using namespace std;

int getLength(Instruction insn) {
	#define OPERATOR_LEN(b, d, l) if(BC_##b == insn) return l;
	FOR_BYTECODES(OPERATOR_LEN)
	#undef OPERATOR_LEN
    	return 1;
}

enum Type {
	INT,
	STRING,
	DOUBLE
};

class VMStack {
private:
	std::vector<pair<int, Type> > stack;
	
	std::vector<double> doubles;
	std::vector<int64_t> ints;
	std::vector<int> strings;
public:
	void push_double(double d) {
		doubles.push_back(d);
		stack.push_back(make_pair(doubles.size() - 1, DOUBLE));
	}

	void push_int(int64_t i) {
		ints.push_back(i);
		stack.push_back(make_pair(ints.size() - 1, INT));
	}

	void push_string(int id) {
		strings.push_back(id);
		stack.push_back(make_pair(strings.size() - 1, STRING));
	}

	double pop_double() {
		int id = stack.back().first;
		stack.pop_back();
		return doubles[id];
	}

	int64_t pop_int() {
		int id = stack.back().first;
		stack.pop_back();
		return ints[id];
	}

	int pop_string() {
		int id = stack.back().first;
		stack.pop_back();
		return strings[id];
	}
	
	void swap() {
		pair<int, Type> upper = stack.back();
		stack.pop_back();
		pair<int, Type> lower = stack.back();
		stack.pop_back();
		stack.push_back(upper);
		stack.push_back(lower);	
	}
	
	void pop() {
		stack.pop_back();
	}
	
};

class Interpreter { 
private:
	Code* _code;
	VMStack stack;
	std::map<int, int64_t> int_vars;
    std::map<int, int> double_vars;
    std::map<int, int> string_vars;
	
	int cmp(double upper, double lower) {
		if (upper < lower) {
			return -1;
		}
		if (upper > lower) {
			return 1;
		}
		return 0;
	}
public:
	Interpreter(Code* code): _code(code) {}
	void execute(BytecodeFunction* function) {
		Bytecode* bc = function->bytecode();
		uint32_t pos = 0;
		while (pos < bc->length()) {
			Instruction insn = bc->getInsn(pos);	
			switch(insn) {
			case BC_DLOAD:
				stack.push_double(bc->getDouble(pos + 1));
				break;
			case BC_ILOAD:
				stack.push_int(bc->getInt64(pos + 1));
				break;
			case BC_SLOAD:
				stack.push_string(bc->getInt16(pos + 1));
				break;
			case BC_DLOAD0:
				stack.push_double(0);
				break;
			case BC_ILOAD0:
				stack.push_int(0);
				break;
			case BC_DLOAD1:
				stack.push_double(1);
				break;
			case BC_ILOAD1:
				stack.push_int(1);
				break;
			case BC_DLOADM1:
				stack.push_double(-1);
				break;
			case BC_ILOADM1:
				stack.push_int(-1);
				break;
			case BC_DADD:		
				stack.push_double(stack.pop_double() + stack.pop_double());
				break;
			case BC_IADD:
				stack.push_int(stack.pop_int() + stack.pop_int());
				break;
			case BC_DSUB:
				stack.push_double(stack.pop_double() - stack.pop_double());
				break;
			case BC_ISUB:
				stack.push_int(stack.pop_int() - stack.pop_int());
				break;
			case BC_DMUL:
				stack.push_double(stack.pop_double() * stack.pop_double());
				break;
			case BC_IMUL:
				stack.push_int(stack.pop_int() * stack.pop_int());
				break;
			case BC_DDIV:
				stack.push_double(stack.pop_double() / stack.pop_double());
				break;
			case BC_IDIV:
				stack.push_int(stack.pop_int() / stack.pop_int());
				break;
			case BC_IMOD:
				stack.push_int(stack.pop_int() % stack.pop_int());
				break;
			case BC_DNEG:
				stack.push_double(-stack.pop_double());
				break;
			case BC_INEG:
				stack.push_int(-stack.pop_int());
				break;
			case BC_IPRINT:
				cout << stack.pop_int();
				break;
			case BC_DPRINT:
				cout << stack.pop_double();
				break;
			case BC_SPRINT:
				cout << _code->constantById(stack.pop_string());
				break;
			case BC_I2D:
				stack.push_double(stack.pop_int());
				break;
			case BC_SWAP:
				stack.swap();
				break;
			case BC_POP:
				stack.pop();
				break;
			case BC_LOADDVAR:
				stack.push_double(double_vars[bc->getInt16(pos + 1)]);
				break;
			case BC_LOADIVAR:
				stack.push_int(int_vars[bc->getInt16(pos + 1)]);
				break;
			case BC_LOADSVAR:
				stack.push_string(string_vars[bc->getInt16(pos + 1)]);
				break;
			case BC_STOREDVAR:
				double_vars[bc->getInt16(pos + 1)] = stack.pop_double();
				break;
			case BC_STOREIVAR:		
				int_vars[bc->getInt16(pos + 1)] = stack.pop_int();
				break;
			case BC_STORESVAR:
				string_vars[bc->getInt16(pos + 1)] = stack.pop_string();
				break;
			case BC_DCMP:
				stack.push_int(cmp(stack.pop_double(), stack.pop_double()));
				break;
			case BC_ICMP:
			{
				int r = cmp(stack.pop_int(), stack.pop_int());
				stack.push_int(r);
				break;
			}
			case BC_JA:
                pos += (bc->getInt16(pos + 1) + 1);
				continue;
            case BC_IFICMPNE:
            	if (stack.pop_int() != stack.pop_int()) {
            		pos += bc->getInt16(pos + 1) + 1;
            		continue; 
            	}
            	break;
            case BC_IFICMPE:
            	if (stack.pop_int() == stack.pop_int()) {
            		pos += bc->getInt16(pos + 1) + 1;
            		continue;
            	}
            	break;
            case BC_IFICMPG:
            	if (stack.pop_int() > stack.pop_int()) {
            		pos += bc->getInt16(pos + 1) + 1;
            		continue;
            	}
            	break;
            case BC_IFICMPGE:
                if (stack.pop_int() >= stack.pop_int()) {
            		pos += bc->getInt16(pos + 1) + 1;
            		continue;
            	}
            	break;
            case BC_IFICMPL:
            	if (stack.pop_int() < stack.pop_int()) {
            		pos += bc->getInt16(pos + 1) + 1;
            		continue;
            	}
            	break;
            case BC_IFICMPLE:
            	if (stack.pop_int() <= stack.pop_int()) {
            		pos += bc->getInt16(pos + 1) + 1;
            		continue;
            	}
            	break;
            case BC_CALL:
            {
                BytecodeFunction* fun = (BytecodeFunction*)_code->functionById(bc->getInt16(pos + 1));
                execute(fun); 
                break;
            }	
            case BC_RETURN:
            	return;				
			default:
				cout << "Unknown instruction   " << insn << '\n';
				assert(0);				
			}
			
			pos += getLength(insn);
		}	
	}	

	
};


