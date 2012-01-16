#include "my_code.h"
#include <stdio.h>
#include <stdlib.h>

using namespace mathvm;

void MyCode::pushDouble (double value) {
	StackValue stackValue;
	stackValue.doubleValue = value;
	stack_.push_back(stackValue);
}

void MyCode::pushInt (int64_t value) {
	StackValue stackValue;
	stackValue.intValue = value;
	stack_.push_back(stackValue);
}

void MyCode::pushString (const char* value) {
	StackValue stackValue;
	stackValue.stringValue = value;
	stack_.push_back(stackValue);
}

void MyCode::pushValue  (StackValue value) {
	stack_.push_back(value);
}

double MyCode::popDouble() {
	StackValue stackValue = stack_.back();
	stack_.pop_back();
	return stackValue.doubleValue;
}

int64_t MyCode::popInt() {
	StackValue stackValue = stack_.back();
	stack_.pop_back();
	return stackValue.intValue;
}

const char*  MyCode::popString() {
	StackValue stackValue = stack_.back();
	stack_.pop_back();
	return stackValue.stringValue;
}

MyCode::StackValue MyCode::popValue() {
	StackValue stackValue = stack_.back();
	stack_.pop_back();
	return stackValue;
}

Status* MyCode::executeBytecode(Bytecode* bytecode) {
	uint32_t insnIndex = 0;
	Instruction curInstruction;

	bool stop = false;
	while (!stop) {
		curInstruction = bytecode->getInsn(insnIndex);
		switch (curInstruction) {
			case BC_INVALID : {
				return new Status("Invalid instruction", insnIndex);
			}
			case BC_DLOAD : {
				double value = bytecode->getDouble(insnIndex + 1);
				pushDouble(value);
				insnIndex += 9;
				break;
			}
			case BC_ILOAD : {
				uint64_t value = bytecode->getInt64(insnIndex + 1);
				pushInt(value);
				insnIndex += 9;
				break;
			}
			case BC_SLOAD : {
				uint16_t strConstantId = bytecode->getUInt16(insnIndex + 1);
				const string& value = constantById(strConstantId);
				pushString(value.c_str());
				insnIndex += 3;
				break;
			}
			case BC_DLOAD0 : {
				pushDouble(0);
				insnIndex += 1;
				break;
			}
			case BC_ILOAD0 : {
				pushInt(0);
				insnIndex += 1;
				break;
			}
			case BC_SLOAD0 : {
				pushString("");
				insnIndex += 1;
				break;
			}
			case BC_DLOAD1 : {
				pushDouble(1);
				insnIndex += 1;
				break;
			}
			case BC_ILOAD1 : {
				pushInt(1);
				insnIndex += 1;
				break;
			}
			case BC_DLOADM1 : {
				pushDouble(-1);
				insnIndex += 1;
				break;
			}
			case BC_ILOADM1 : {
				pushInt(-1);
				insnIndex += 1;
				break;
			}
			case BC_DADD : {
				double upper = popDouble();
				double lower = popDouble();
				pushDouble(upper + lower);
				insnIndex += 1;
				break;
			}
			case BC_IADD : {
				int64_t upper = popInt();
				int64_t lower = popInt();
				pushInt(upper + lower);
				insnIndex += 1;
				break;
			}
			case BC_DSUB : {
				double upper = popDouble();
				double lower = popDouble();
				pushDouble(upper - lower);
				insnIndex += 1;
				break;
			}
			case BC_ISUB : {
				int64_t upper = popInt();
				int64_t lower = popInt();
				pushInt(upper - lower);
				insnIndex += 1;
				break;
			}
			case BC_DMUL : {
				double upper = popDouble();
				double lower = popDouble();
				pushDouble(upper * lower);
				insnIndex += 1;
				break;
			}
			case BC_IMUL : {
				int64_t upper = popInt();
				int64_t lower = popInt();
				pushInt(upper * lower);
				insnIndex += 1;
				break;
			}
			case BC_DDIV : {
				double upper = popDouble();
				double lower = popDouble();
				pushDouble(upper / lower);
				insnIndex += 1;
				break;
			}
			case BC_IDIV : {
				int64_t upper = popInt();
				int64_t lower = popInt();
				pushInt(upper / lower);
				insnIndex += 1;
				break;
			}
			case BC_IMOD : {
				int64_t upper = popInt();
				int64_t lower = popInt();
				pushInt(upper % lower);
				insnIndex += 1;
				break;
			}
			case BC_DNEG : {
				double upper = popDouble();
				pushDouble(-upper);
				insnIndex += 1;
				break;
			}
			case BC_INEG : {
				int64_t upper = popInt();
				pushInt(-upper);
				insnIndex += 1;
				break;
			}
			case BC_IPRINT : {
				int64_t upper = popInt();
				cout << upper;
				insnIndex += 1;
				break;
			}
			case BC_DPRINT : {
				double upper = popDouble();
				cout << upper;
				insnIndex += 1;
				break;
			}
			case BC_SPRINT : {
				const char* upper = popString();
				cout << upper;
				insnIndex += 1;
				break;
			}
			case BC_I2D : {
				uint64_t upper = popInt();
				pushDouble(double(upper));
				insnIndex += 1;
				break;
			}
			case BC_D2I : {
				double upper = popDouble();
				pushInt(uint64_t(upper));
				insnIndex += 1;
				break;
			}
			case BC_S2I : {
				const char* upper = popString();
				int value = atoi(upper);
				pushInt(uint64_t(value));
				insnIndex += 1;
				break;
			}
			case BC_SWAP : {
				StackValue upper = popValue();
				StackValue lower = popValue();
				pushValue(upper);
				pushValue(lower);
				insnIndex += 1;
				break;
			}
			case BC_POP : {
				popValue();
				insnIndex += 1;
				break;
			}
			case BC_LOADDVAR : {
				uint16_t varId = bytecode->getUInt16(insnIndex + 1);
				if (varMap_.count(varId) == 0) {
					return new Status("Variable not found", insnIndex);
				}
				double value = varMap_[varId]->getDoubleValue();
				pushDouble(value);
				insnIndex += 3;
				break;
			}
			case BC_LOADIVAR : {
				uint16_t varId = bytecode->getUInt16(insnIndex + 1);
				if (varMap_.count(varId) == 0) {
					return new Status("Variable not found", insnIndex);
				}
				int value = varMap_[varId]->getIntValue();
				pushInt(value);
				insnIndex += 3;
				break;
			}
			case BC_LOADSVAR : {
				uint16_t varId = bytecode->getUInt16(insnIndex + 1);
				if (varMap_.count(varId) == 0) {
					return new Status("Variable not found", insnIndex);
				}
				const char* value = varMap_[varId]->getStringValue();
				pushString(value);
				insnIndex += 3;
				break;
			}
			case BC_STOREDVAR : {
				double value = popDouble();
				uint16_t varId = bytecode->getUInt16(insnIndex + 1);
				if (varMap_.count(varId) == 0) {
					Var* newVar = new Var(VT_DOUBLE, "");
					varMap_[varId] = newVar;
				}
				varMap_[varId]->setDoubleValue(value);
				insnIndex += 3;
				break;
			}
			case BC_STOREIVAR : {
				int value = popInt();
				uint16_t varId = bytecode->getUInt16(insnIndex + 1);
				if (varMap_.count(varId) == 0) {
					Var* newVar = new Var(VT_INT, "");
					varMap_[varId] = newVar;
				}
				varMap_[varId]->setIntValue(value);
				insnIndex += 3;
				break;
			}
			case BC_STORESVAR : {
				const char* value = popString();
				uint16_t varId = bytecode->getUInt16(insnIndex + 1);
				if (varMap_.count(varId) == 0) {
					Var* newVar = new Var(VT_STRING, "");
					varMap_[varId] = newVar;
				}
				varMap_[varId]->setStringValue(value);
				insnIndex += 3;
				break;
			}
			case BC_DCMP : {
				double upper = popDouble();
				double lower = popDouble();
				uint64_t result;
				if (upper == lower) {
					result = 0;
				} else if (upper < lower) {
					result = 1;
				} else {
					result = -1;
				}
				pushInt(result);
				insnIndex += 1;
				break;
			}
			case BC_ICMP : {
				int64_t upper = popInt();
				int64_t lower = popInt();
				uint64_t result;
				if (upper == lower) {
					result = 0;
				} else if (upper < lower) {
					result = 1;
				} else {
					result = -1;
				}
				pushInt(result);
				insnIndex += 1;
				break;
			}
			case BC_JA : {
				int16_t offset = bytecode->getInt16(insnIndex + 1);
				insnIndex += 1 + offset;
				break;
			}
			case BC_IFICMPNE : {
				int16_t offset = bytecode->getInt16(insnIndex + 1);
				int64_t upper = popInt();
				int64_t lower = popInt();
				if (upper != lower)
					insnIndex += 1 + offset;
				else
					insnIndex += 3;
				break;
			}
			case BC_IFICMPE : {
				int16_t offset = bytecode->getInt16(insnIndex + 1);
				int64_t upper = popInt();
				int64_t lower = popInt();
				if (upper == lower)
					insnIndex += 1 + offset;
				else
					insnIndex += 3;
				break;
			}
			case BC_IFICMPG : {
				int16_t offset = bytecode->getInt16(insnIndex + 1);
				int64_t upper = popInt();
				int64_t lower = popInt();
				if (upper > lower)
					insnIndex += 1 + offset;
				else
					insnIndex += 3;
				break;
			}
			case BC_IFICMPGE : {
				int16_t offset = bytecode->getInt16(insnIndex + 1);
				int64_t upper = popInt();
				int64_t lower = popInt();
				if (upper >= lower)
					insnIndex += 1 + offset;
				else
					insnIndex += 3;
				break;
			}
			case BC_IFICMPL : {
				int16_t offset = bytecode->getInt16(insnIndex + 1);
				int64_t upper = popInt();
				int64_t lower = popInt();
				if (upper < lower)
					insnIndex += 1 + offset;
				else
					insnIndex += 3;
				break;
			}
			case BC_IFICMPLE : {
				int16_t offset = bytecode->getInt16(insnIndex + 1);
				int64_t upper = popInt();
				int64_t lower = popInt();
				if (upper <= lower)
					insnIndex += 1 + offset;
				else
					insnIndex += 3;
				break;
			}
			case BC_STOP : {
				stop = true;
				insnIndex += 1;
				break;
			}
			case BC_CALL : {
				uint16_t funcId = bytecode->getUInt16(insnIndex + 1);
				BytecodeFunction* bytecodeFunc = (BytecodeFunction*)functionById(funcId);
				if (bytecodeFunc == 0) {
					return new Status("Function not found", insnIndex);
				}
				Status* status = executeBytecode(bytecodeFunc->bytecode());
				if (status != NULL) {
					return status;
				}

				insnIndex += 3;
				break;
			}
			case BC_CALLNATIVE : {
				return new Status("Native functions not supported yet", insnIndex);
				//uint16_t nativeFuncId = bytecode->getUInt16(insnIndex + 1);
				//insnIndex += 3;
				//break;
			}
			case BC_RETURN : {
				return NULL;
			}

			default : {
				return new Status("Not handled instruction", insnIndex);
			}


		/*
		DO(CALLNATIVE, "Call native function, next two bytes - id of the native function.", 3)  \
		 */
		}
	}

	return NULL;
}

Status* MyCode::execute(vector<Var*>& vars) {
	BytecodeFunction* pseudoFunction = (BytecodeFunction*)functionByName(AstFunction::top_name);
	Bytecode* bytecode = pseudoFunction->bytecode();
	return executeBytecode(bytecode);
}


