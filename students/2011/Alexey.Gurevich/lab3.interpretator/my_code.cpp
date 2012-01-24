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
			case BC_LOADCTXDVAR : {
				uint16_t ctxId = bytecode->getUInt16(insnIndex + 1);
				uint16_t varId = bytecode->getUInt16(insnIndex + 3);
				if (varsStorage_[ctxId][varId] == 0) {
					return new Status("Variable not found", insnIndex);
				}
				double value = varsStorage_[ctxId][varId]->getDoubleValue();
				pushDouble(value);
				insnIndex += 5;
				break;
			}
			case BC_LOADCTXIVAR : {
				uint16_t ctxId = bytecode->getUInt16(insnIndex + 1);
				uint16_t varId = bytecode->getUInt16(insnIndex + 3);
				if (varsStorage_[ctxId][varId] == 0) {
					return new Status("Variable not found", insnIndex);
				}
				int64_t value = varsStorage_[ctxId][varId]->getIntValue();
				pushInt(value);
				insnIndex += 5;
				break;
			}
			case BC_LOADCTXSVAR : {
				uint16_t ctxId = bytecode->getUInt16(insnIndex + 1);
				uint16_t varId = bytecode->getUInt16(insnIndex + 3);
				if (varsStorage_[ctxId][varId] == 0) {
					return new Status("Variable not found", insnIndex);
				}
				const char* value = varsStorage_[ctxId][varId]->getStringValue();
				pushString(value);
				insnIndex += 5;
				break;
			}
			case BC_STORECTXDVAR : {
				double value = popDouble();
				uint16_t ctxId = bytecode->getUInt16(insnIndex + 1);
				uint16_t varId = bytecode->getUInt16(insnIndex + 3);
				if (varsStorage_[ctxId][varId] == 0) {
					Var* newVar = new Var(VT_DOUBLE, "");
					varsStorage_[ctxId][varId] = newVar;
				}
				varsStorage_[ctxId][varId]->setDoubleValue(value);
				insnIndex += 5;
				break;
			}
			case BC_STORECTXIVAR : {
				int64_t value = popInt();
				uint16_t ctxId = bytecode->getUInt16(insnIndex + 1);
				uint16_t varId = bytecode->getUInt16(insnIndex + 3);
				if (varsStorage_[ctxId][varId] == 0) {
					Var* newVar = new Var(VT_INT, "");
					varsStorage_[ctxId][varId] = newVar;
				}
				varsStorage_[ctxId][varId]->setIntValue(value);
				insnIndex += 5;
				break;
			}
			case BC_STORECTXSVAR : {
				const char* value = popString();
				uint16_t ctxId = bytecode->getUInt16(insnIndex + 1);
				uint16_t varId = bytecode->getUInt16(insnIndex + 3);
				if (varsStorage_[ctxId][varId] == 0) {
					Var* newVar = new Var(VT_STRING, "");
					varsStorage_[ctxId][varId] = newVar;
				}
				varsStorage_[ctxId][varId]->setStringValue(value);
				insnIndex += 5;
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

				func_vars_t prev_block = varsStorage_[funcId];
				varsStorage_[funcId] = new var_ptr_t[funcVarsCount_[funcId]];
				memset(varsStorage_[funcId], 0, funcVarsCount_[funcId] * sizeof(var_ptr_t));

				Status* status = executeBytecode(bytecodeFunc->bytecode());

				for (uint16_t i = 0; i != funcVarsCount_[funcId]; ++i)
					delete varsStorage_[funcId][i];
				delete [] varsStorage_[funcId];
				varsStorage_[funcId] = prev_block;

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
		}
	}

	return NULL;
}

Status* MyCode::execute(vector<Var*>& vars) {
	// preparing variables storage
	uint16_t funcNumber = funcVarsCount_.size();
	varsStorage_ = new func_vars_t[funcNumber];
	memset(varsStorage_, 0, funcNumber * sizeof(func_vars_t));
	varsStorage_[0] = new var_ptr_t[funcVarsCount_[0]];
	memset(varsStorage_[0], 0, funcVarsCount_[0] * sizeof(var_ptr_t));

	BytecodeFunction* pseudoFunction = (BytecodeFunction*)functionByName(AstFunction::top_name);
	Bytecode* bytecode = pseudoFunction->bytecode();

	Status* executionStatus = executeBytecode(bytecode);

	for (uint16_t i = 0; i != funcVarsCount_[0]; ++i)
		delete varsStorage_[0][i];
	delete [] varsStorage_[0];
	delete [] varsStorage_;

	return executionStatus;
}


