#ifndef _EXEC_HPP_
#define _EXEC_HPP_

#include <vector>
#include <cstring>
#include <map>
#include <ostream>

#include "ast.h"

using namespace mathvm;


union StackValueType {
	double dv;
	int64_t iv;
	const char * sv;


	StackValueType(double d): dv(d) {}


	StackValueType(int64_t i): iv(i) {}


	StackValueType(const char * s): sv(s) {}


	operator double() {
		return dv;
	}

	operator int64_t() {
		return iv;
	}

	operator const char *() {
		return sv;
	}	
};


class CodeExecutor: public Code {

typedef std::map <uint16_t, std::vector<Var *> >  VarsInContextsT;

private:
	std::vector <StackValueType> stack;

	std::ostream & os;

	
	std::map <uint16_t, uint16_t> contextVarIds;
	VarsInContextsT varInContexts;


	Status * executeBytecode(Bytecode * bytecode) {
		uint32_t index = 0;

		while (true) {
			Instruction instruction = bytecode->getInsn(index);

			if (instruction == BC_INVALID) {
				return new Status("Invalid instruction", index);
			}
			else if (instruction == BC_DLOAD) {
				stack.push_back( bytecode->getDouble(index + 1) );
				index += 9;
			} 			
			else if (instruction == BC_ILOAD) {
				stack.push_back( bytecode->getInt64(index + 1) );
				index += 9;
			}
			else if (instruction == BC_SLOAD) {
				const string& value = constantById(bytecode->getUInt16(index + 1));
				stack.push_back( value.c_str() );
				index += 3;
			}
			else if (instruction == BC_DLOAD0) {
				stack.push_back( 0.0 );
				index += 1;
			}
			else if (instruction == BC_ILOAD0) {
				stack.push_back( (int64_t) 0 );
				index += 1;
			}
			else if (instruction == BC_SLOAD0) {
				stack.push_back( "" );
				index += 1;
			}
			else if (instruction == BC_DLOAD1) {
				stack.push_back( 1.0 );
				index += 1;
			}
			else if (instruction == BC_ILOAD1) {
				stack.push_back( (int64_t) 1 );
				index += 1;
			}
			else if (instruction == BC_DLOADM1) {
				stack.push_back( -1.0 );
				index += 1;
			}
			else if (instruction == BC_ILOADM1) {
				stack.push_back( (int64_t) -1 );
				index += 1;
			}
			else if (instruction == BC_DADD) {
				double first = (double) stack.back();
				stack.pop_back();
				double second = (double) stack.back();
				stack.pop_back();
				
				stack.push_back(first + second);
				index += 1;
			}
			else if (instruction == BC_IADD) {
				int64_t first = (int64_t) stack.back();
				stack.pop_back();
				int64_t second = (int64_t) stack.back();
				stack.pop_back();
				
				stack.push_back(first + second);
				index += 1;
			}
			else if (instruction == BC_DSUB) {
				double first = (double) stack.back();
				stack.pop_back();
				double second = (double) stack.back();
				stack.pop_back();
				
				stack.push_back(first - second);
				index += 1;
			}
			else if (instruction == BC_ISUB) {
				int64_t first = (int64_t) stack.back();
				stack.pop_back();
				int64_t second = (int64_t) stack.back();
				stack.pop_back();
				
				stack.push_back(first - second);
				index += 1;
			}
			else if (instruction == BC_DMUL) {
				double first = (double) stack.back();
				stack.pop_back();
				double second = (double) stack.back();
				stack.pop_back();
				
				stack.push_back(first * second);
				index += 1;
			}
			else if (instruction == BC_IMUL) {
				int64_t first = (int64_t) stack.back();
				stack.pop_back();
				int64_t second = (int64_t) stack.back();
				stack.pop_back();
				
				stack.push_back(first * second);
				index += 1;
			}
			else if (instruction == BC_DDIV) {
				double first = (double) stack.back();
				stack.pop_back();
				double second = (double) stack.back();
				stack.pop_back();
				
				stack.push_back(first / second);
				index += 1;
			}
			else if (instruction == BC_IDIV) {
				int64_t first = (int64_t) stack.back();
				stack.pop_back();
				int64_t second = (int64_t) stack.back();
				stack.pop_back();
				
				stack.push_back(first / second);
				index += 1;
			}
			else if (instruction == BC_IMOD) {
				int64_t first = (int64_t) stack.back();
				stack.pop_back();
				int64_t second = (int64_t) stack.back();
				stack.pop_back();
				
				stack.push_back(first % second);
				index += 1;
			}
			else if (instruction == BC_DNEG) {
				double value = (double) stack.back();
				stack.pop_back();
				stack.push_back(-value);
				index += 1;
			}
			else if (instruction == BC_INEG) {
				int64_t value = (int64_t) stack.back();
				stack.pop_back();
				stack.push_back(-value);
				index += 1;
			}
			else if (instruction == BC_IPRINT) {
				os << (int64_t) stack.back();
				stack.pop_back();
				index += 1;
			}
			else if (instruction == BC_DPRINT) {
				os << (double) stack.back();
				stack.pop_back();
				index += 1;
			}
			else if (instruction == BC_SPRINT) {
				os << (const char *) stack.back();
				stack.pop_back();
				index += 1;
			}
			else if (instruction == BC_I2D) {
				double value = (double) (int64_t) stack.back();
				stack.pop_back();
				stack.push_back(value);				
				index += 1;
			}
			else if (instruction == BC_D2I) {
				int64_t value = (int64_t) (double) stack.back();
				stack.pop_back();
				stack.push_back(value);				
				index += 1;
			}
			else if (instruction == BC_S2I) {
				int64_t value = strlen((const char *) stack.back());
				stack.pop_back();
				stack.push_back(value);				
				index += 1;
			}
			else if (instruction == BC_SWAP) {
				StackValueType first = stack.back();
				stack.pop_back();
				StackValueType second = stack.back();
				stack.pop_back();

				stack.push_back(second);
				stack.push_back(first);
				index += 1;
			}
			else if (instruction == BC_POP) {
				stack.pop_back();
				index += 1;
			}
			else if (instruction == BC_LOADCTXDVAR) {
				uint16_t contextId = bytecode->getUInt16(index + 1);
				uint16_t varId = bytecode->getUInt16(index + 3);

				VarsInContextsT::iterator context = varInContexts.find(contextId);
				if (context == varInContexts.end()) {
					return new Status("Ivalid context", index);
				}
				Var * var = context->second.at(varId);

				if (var == 0) {
					return new Status("Invalid variable", index);
				}
	
				stack.push_back(var->getDoubleValue());
				index += 5;
			}
			else if (instruction == BC_LOADCTXIVAR) {
				uint16_t contextId = bytecode->getUInt16(index + 1);
				uint16_t varId = bytecode->getUInt16(index + 3);

				VarsInContextsT::iterator context = varInContexts.find(contextId);
				if (context == varInContexts.end()) {
					return new Status("Ivalid context", index);
				}
				Var * var = context->second.at(varId);

				if (var == 0) {
					return new Status("Invalid variable", index);
				}
	
				stack.push_back(var->getIntValue());
				index += 5;
			}
			else if (instruction == BC_LOADCTXSVAR) {
				uint16_t contextId = bytecode->getUInt16(index + 1);
				uint16_t varId = bytecode->getUInt16(index + 3);

				VarsInContextsT::iterator context = varInContexts.find(contextId);
				if (context == varInContexts.end()) {
					return new Status("Ivalid context", index);
				}
				Var * var = context->second.at(varId);

				if (var == 0) {
					return new Status("Invalid variable", index);
				}
	
				stack.push_back(var->getStringValue());
				index += 5;
			}
			else if (instruction == BC_STORECTXDVAR) {
				uint16_t contextId = bytecode->getUInt16(index + 1);
				uint16_t varId = bytecode->getUInt16(index + 3);

				VarsInContextsT::iterator context = varInContexts.find(contextId);
				if (context == varInContexts.end()) {
					return new Status("Ivalid context", index);
				}
				Var * & var = context->second.at(varId);

				if (var == 0) {
					var = new Var(VT_DOUBLE, "");

					if (context->second.at(varId) == 0) {
						os << "\nSHIT\n";
					}
				}

				var->setDoubleValue((double) stack.back());
				stack.pop_back();
				index += 5;
			}
			else if (instruction == BC_STORECTXIVAR) {
				uint16_t contextId = bytecode->getUInt16(index + 1);
				uint16_t varId = bytecode->getUInt16(index + 3);

				VarsInContextsT::iterator context = varInContexts.find(contextId);
				if (context == varInContexts.end()) {
					return new Status("Ivalid context", index);
				}
				Var * & var = context->second.at(varId);

				if (var == 0) {
					var = new Var(VT_INT, "");

					if (context->second.at(varId) == 0) {
						os << "\nSHIT\n";
					}
				}

				var->setIntValue((int64_t) stack.back());
				stack.pop_back();
				index += 5;
			}
			else if (instruction == BC_STORECTXSVAR) {
				uint16_t contextId = bytecode->getUInt16(index + 1);
				uint16_t varId = bytecode->getUInt16(index + 3);

				VarsInContextsT::iterator context = varInContexts.find(contextId);
				if (context == varInContexts.end()) {
					return new Status("Ivalid context", index);
				}
				Var * & var = context->second.at(varId);

				if (var == 0) {
					var = new Var(VT_STRING, "");

					if (context->second.at(varId) == 0) {
						os << "\nSHIT\n";
					}
				}

				var->setStringValue((const char *) stack.back());
				stack.pop_back();
				index += 5;
			}
			else if (instruction == BC_DCMP) {
				double first = (double) stack.back();
				stack.pop_back();
				double second = (double) stack.back();
				stack.pop_back();

				if (first == second) {
					stack.push_back( (int64_t) 0 );
				} 
				else if (first < second) {
					stack.push_back( (int64_t) 1 );
				} 
				else {
					stack.push_back( (int64_t) -1 );
				}

				index += 1;
			}
			else if (instruction == BC_ICMP) {
				int64_t first = (int64_t) stack.back();
				stack.pop_back();
				int64_t second = (int64_t) stack.back();
				stack.pop_back();

				if (first == second) {
					stack.push_back( (int64_t) 0 );
				} 
				else if (first < second) {
					stack.push_back( (int64_t) 1 );
				} 
				else {
					stack.push_back( (int64_t) -1 );
				}

				index += 1;
			}
			else if (instruction == BC_JA) {
				index += 1 + bytecode->getInt16(index + 1);
			}
			else if (instruction == BC_IFICMPNE) {
				int64_t first = (int64_t) stack.back();
				stack.pop_back();
				int64_t second = (int64_t) stack.back();
				stack.pop_back();

				if (first != second) {
					index += 1 + bytecode->getInt16(index + 1);
				}
				else {
					index += 3;
				}
			}
			else if (instruction == BC_IFICMPE) {
				int64_t first = (int64_t) stack.back();
				stack.pop_back();
				int64_t second = (int64_t) stack.back();
				stack.pop_back();

				if (first == second) {
					index += 1 + bytecode->getInt16(index + 1);
				}
				else {
					index += 3;
				}
			}
			else if (instruction == BC_IFICMPG) {
				int64_t first = (int64_t) stack.back();
				stack.pop_back();
				int64_t second = (int64_t) stack.back();
				stack.pop_back();

				if (first > second) {
					index += 1 + bytecode->getInt16(index + 1);
				}
				else {
					index += 3;
				}
			}
			else if (instruction == BC_IFICMPGE) {
				int64_t first = (int64_t) stack.back();
				stack.pop_back();
				int64_t second = (int64_t) stack.back();
				stack.pop_back();

				if (first >= second) {
					index += 1 + bytecode->getInt16(index + 1);
				}
				else {
					index += 3;
				}
			}
			else if (instruction == BC_IFICMPL) {
				int64_t first = (int64_t) stack.back();
				stack.pop_back();
				int64_t second = (int64_t) stack.back();
				stack.pop_back();

				if (first < second) {
					index += 1 + bytecode->getInt16(index + 1);
				}
				else {
					index += 3;
				}
			}
			else if (instruction == BC_IFICMPLE) {
				int64_t first = (int64_t) stack.back();
				stack.pop_back();
				int64_t second = (int64_t) stack.back();
				stack.pop_back();

				if (first <= second) {
					index += 1 + bytecode->getInt16(index + 1);
				}
				else {
					index += 3;
				}
			}
			else if (instruction == BC_STOP) {
				index += 1;
				break;
			}
			else if (instruction == BC_CALL) {
				uint16_t functionId = bytecode->getUInt16(index + 1);
				BytecodeFunction * bytecodeFuncion = (BytecodeFunction *) functionById(functionId);

				if (bytecodeFuncion == 0) {
					return new Status("Invalid function", index);
				}

				VarsInContextsT::iterator context = varInContexts.find(functionId);
				if (context == varInContexts.end()) {
					return new Status("Ivalid context in call", index);
				}
				std::vector<Var *> currentContext = context->second;
				varInContexts.erase(context);
				
				varInContexts.insert(std::make_pair( functionId, std::vector<Var *>(contextVarIds[functionId]) ));

				Status * status = executeBytecode(bytecodeFuncion->bytecode());

				context = varInContexts.find(functionId);
				varInContexts.erase(context);
				varInContexts.insert(std::make_pair( functionId, currentContext ));

				if (status != 0) {
					return status;
				}

				index += 3;
			}
			else if (instruction == BC_RETURN) {
				return 0;
			}
			else {
				return new Status("Unsupported instruction", index);
			}
			if (!stack.empty()) {
			//	std::cerr << (double) stack.back() << " ===== " << (int64_t) stack.back() << "\n";
			}
		}
		return 0;
	}
	

public:
	CodeExecutor(std::ostream& s): os(s) {}


	void setContextVarIds(std::map <uint16_t, uint16_t> varIds) {
		contextVarIds = varIds;
	}


	Status * execute(vector <Var *> & vars) {
		for (std::map <uint16_t, uint16_t>::iterator functionIter = contextVarIds.begin(); functionIter != contextVarIds.end(); ++functionIter) {
			varInContexts.insert(std::make_pair( functionIter->first, std::vector<Var *>(functionIter->second) ));		
		}

		BytecodeFunction * mainFunction = (BytecodeFunction *) functionByName(AstFunction::top_name);
		Bytecode * bytecode = mainFunction->bytecode();

		return executeBytecode(bytecode);
	}
};



#endif /* _EXEC_HPP_ */


