#include "bytecoder.h"
#include "my_translator.h"
#include "mathvm.h"
#include "parser.h"
#include <iostream>
#include <iomanip>
#include <stack>

using namespace AsmJit;

FileLogger logger(stderr);

namespace mathvm {

Translator* Translator::create(const string& impl) {
	if (impl == "my_jit") {
		return new MyJitTranslator();
	}
	assert(false);
	return 0;
}

///// start of auxiliary functions //////
void printInt(sysint_t value) {
	std::cout << value;
}

void printDouble(double value) {
	std::cout << value;
}

void printString(sysint_t strPtr) {
	const string* value = (string*) strPtr;
	std::cout << value->c_str();
}

sysint_t idiv(sysint_t upper, sysint_t lower) {
	return upper / lower;
}

sysint_t imod(sysint_t upper, sysint_t lower) {
	return upper % lower;
}

double i2d(sysint_t value) {
	return (double) value;
}

sysint_t d2i(double value) {
	return (sysint_t) value;
}

sysint_t s2i(sysint_t strPtr) {
	const string* value = (string*) strPtr;
	return (sysint_t)atoi(value->c_str());
}

sysint_t icmp(sysint_t upper, sysint_t lower) {
	sysint_t result;
	if (upper == lower) {
		result = 0;
	} else if (upper < lower) {
		result = 1;
	} else {
		result = -1;
	}
	return result;
}

sysint_t dcmp(double upper, double lower) {
	sysint_t result;
	if (upper == lower) {
		result = 0;
	} else if (upper < lower) {
		result = 1;
	} else {
		result = -1;
	}
	return result;
}
///// end of auxiliary functions //////


///// start of useful functions for simplification of pop, push, etc /////
void pushDouble(double value, Compiler* *compiler) {
	sysint_t nvalue = *((sysint_t*) &value);
	// unfortunately, simple  "compiler->push(imm(nvalue));" doesn't work
	GPVar intVar((*compiler)->newGP());
	(*compiler)->mov(intVar, imm(nvalue));
	(*compiler)->push(intVar);
	(*compiler)->unuse(intVar);
}

void pushXMMVar(XMMVar& doubleVar, Compiler* *compiler) {
	GPVar tmp((*compiler)->newGP());
	(*compiler)->movq(tmp, doubleVar);
	(*compiler)->push(tmp);
	(*compiler)->unuse(tmp);
	(*compiler)->unuse(doubleVar);
}

XMMVar popXMMVar(Compiler* *compiler) {
	GPVar tmp((*compiler)->newGP());
	XMMVar doubleVar((*compiler)->newXMM());
	(*compiler)->pop(tmp);
	(*compiler)->movq(doubleVar, tmp);
	(*compiler)->unuse(tmp);
	return doubleVar;
}

Mem getVarPtr (Compiler* *compiler, uint16_t ctxId, uint16_t varId, MyJitTranslator::func_vars_t* varsStorage) {
	GPVar ctxPtr((*compiler)->newGP());
	GPVar ctxAddr((*compiler)->newGP());
	(*compiler)->mov(ctxAddr, imm( (sysint_t) &(varsStorage[ctxId])));
	(*compiler)->mov(ctxPtr, qword_ptr(ctxAddr));
	(*compiler)->unuse(ctxAddr);
	Mem varPtr = qword_ptr(ctxPtr, varId * sizeof(sysint_t));
	(*compiler)->unuse(ctxPtr);
	return varPtr;
}

void handleIFICMP(Compiler* *compiler, map<uint32_t, AsmJit::Label*>& labelMap,
		Bytecode* bytecode, uint32_t insnIndex, Instruction const& curICMPinsn) {

	int16_t offset = bytecode->getInt16(insnIndex + 1);
	uint32_t jumpInsn = insnIndex + 1 + offset;

	AsmJit::Label* label = labelMap[jumpInsn];

	GPVar upper((*compiler)->newGP());
	(*compiler)->pop(upper);
	GPVar lower((*compiler)->newGP());
	(*compiler)->pop(lower);

	(*compiler)->cmp(upper, lower);

	switch (curICMPinsn) {
		case BC_IFICMPE  : (*compiler)->je(*label); break;
		case BC_IFICMPNE : (*compiler)->jne(*label); break;
		case BC_IFICMPG  : (*compiler)->jg(*label); break;
		case BC_IFICMPGE : (*compiler)->jge(*label); break;
		case BC_IFICMPL  : (*compiler)->jl(*label); break;
		case BC_IFICMPLE : (*compiler)->jle(*label); break;
		default:
			assert(false);
			break;
	}

	(*compiler)->unuse(upper);
	(*compiler)->unuse(lower);
}
///// end of useful functions for simplification of pop, push, etc /////

///// start of functions for handling JUMPs /////
void storeLabels(Compiler* *compiler, map<uint32_t, AsmJit::Label*> *labelMap, Bytecode* bytecode) {
	uint32_t insnIndex = 0;
	Instruction curInstruction;
	bool stop = false;
	while (!stop && (insnIndex < bytecode->length())) {
		curInstruction = bytecode->getInsn(insnIndex);
		switch (curInstruction) {
			case BC_INVALID : {
				return;
			}
			case BC_DLOAD :
			case BC_ILOAD : {
				insnIndex += 9;
				break;
			}
			case BC_SLOAD : {
				insnIndex += 3;
				break;
			}
			case BC_DLOAD0  :
			case BC_ILOAD0  :
			case BC_SLOAD0  :
			case BC_DLOAD1  :
			case BC_ILOAD1  :
			case BC_DLOADM1 :
			case BC_ILOADM1 :
			case BC_DADD    :
			case BC_IADD    :
			case BC_DSUB    :
			case BC_ISUB    :
			case BC_DMUL    :
			case BC_IMUL    :
			case BC_DDIV    :
			case BC_IDIV    :
			case BC_IMOD    :
			case BC_DNEG    :
			case BC_INEG    :
			case BC_IPRINT  :
			case BC_DPRINT  :
			case BC_SPRINT  :
			case BC_I2D     :
			case BC_D2I     :
			case BC_S2I     :
			case BC_SWAP    :
			case BC_POP     : {
				insnIndex += 1;
				break;
			}
			case BC_LOADCTXDVAR  :
			case BC_LOADCTXIVAR  :
			case BC_LOADCTXSVAR  :
			case BC_STORECTXDVAR :
			case BC_STORECTXIVAR :
			case BC_STORECTXSVAR : {
				insnIndex += 5;
				break;
			}
			case BC_DCMP :
			case BC_ICMP : {
				insnIndex += 1;
				break;
			}
			case BC_JA :
			case BC_IFICMPNE :
			case BC_IFICMPE  :
			case BC_IFICMPG  :
			case BC_IFICMPGE :
			case BC_IFICMPL  :
			case BC_IFICMPLE : {

				int16_t offset = bytecode->getInt16(insnIndex + 1);
				uint32_t jumpInsn = insnIndex + 1 + offset;
				if (labelMap->count(jumpInsn) == 0) {
					(*labelMap)[jumpInsn] = new AsmJit::Label((*compiler)->newLabel());
				}

				insnIndex += 3;
				break;
			}
			case BC_STOP : {
				stop = true;
				insnIndex += 1;
				break;
			}
			case BC_CALL 	   :
			case BC_CALLNATIVE : {
				insnIndex += 3;
				break;
			}
			case BC_RETURN : {
				insnIndex += 1;
				break;
			}
			default : {
				return;
			}
		}
	}
}

void freeLabelMap(map<uint32_t, AsmJit::Label*> *labelMap) {
	for (map<uint32_t, AsmJit::Label*>::iterator it=labelMap->begin() ; it != labelMap->end(); ++it) {
		delete it->second;
	}
}

void checkJumps(Compiler* *compiler, map<uint32_t, AsmJit::Label*> *labelMap, uint32_t insnIndex) {
	if (labelMap->count(insnIndex) != 0) {
		AsmJit::Label* label = (*labelMap)[insnIndex];
		(*compiler)->bind(*label);
	}
}
///// end of functions for handling JUMPs /////

// useful function builder
FunctionBuilderX fillFunctionBuilderX(BytecodeFunction* bytecodeFunction) {
	FunctionBuilderX functionBuilderX;
	// setting types of arguments
	for (uint16_t i = 0; i != bytecodeFunction->parametersNumber(); ++i) {
		if (bytecodeFunction->parameterType(i) == VT_DOUBLE)
			functionBuilderX.addArgument<double>();
		else
			functionBuilderX.addArgument<sysint_t>();
	}
	// setting type of return value
	if (bytecodeFunction->returnType() == VT_VOID) {
		functionBuilderX.setReturnValue<void>();
	}
	else if (bytecodeFunction->returnType() == VT_DOUBLE) {
		functionBuilderX.setReturnValue<double>();
	}
	else {
		functionBuilderX.setReturnValue<sysint_t>();
	}
	return functionBuilderX;
}


//// MAIN FUNCTION to translate BYTECODE into MACHCODE /////
Status* MyJitTranslator::generateFunction(Code* code, BytecodeFunction* bytecodeFunction, void* *function) {

	Bytecode* bytecode = bytecodeFunction->bytecode();
	Compiler* compiler = new Compiler;
	if (SET_LOGGER)
		compiler->setLogger(&logger);

	FunctionBuilderX mainFunctionBuilder = fillFunctionBuilderX(bytecodeFunction);
	compiler->newFunction(CALL_CONV_DEFAULT, mainFunctionBuilder);
	// Try to generate function without prolog/epilog code:
	compiler->getFunction()->setHint(FUNCTION_HINT_NAKED, true);
	compiler->comment("function %s", bytecodeFunction->name().c_str());

	// for parameters storing
	uint16_t curParameter = bytecodeFunction->parametersNumber();

	// for BC_SWAP and BC_POP
	stack<VarType> typeStack;

	// for jumps
	map<uint32_t, AsmJit::Label*> labelMap;
	storeLabels(&compiler, &labelMap, bytecode);

	uint32_t insnIndex = 0;
	Instruction curInstruction;
	bool stop = false;
	while (!stop && (insnIndex < bytecode->length())) {
		curInstruction = bytecode->getInsn(insnIndex);
		checkJumps(&compiler, &labelMap, insnIndex);

		switch (curInstruction) {
			case BC_INVALID : {
				freeLabelMap(&labelMap);
				return new Status("Invalid instruction", insnIndex);
			}
			case BC_DLOAD : {
				double value = bytecode->getDouble(insnIndex + 1);
				pushDouble(value, &compiler);
				
				typeStack.push(VT_DOUBLE);
				insnIndex += 9;
				break;
			}
			case BC_ILOAD : {
				sysint_t value = bytecode->getInt64(insnIndex + 1);

				// unfortunately, simple  "compiler->push(imm(value));" doesn't work with large numbers (e.g. add.mvm test)
				GPVar intVar(compiler->newGP());
				compiler->mov(intVar, imm(value));
				compiler->push(intVar);
				compiler->unuse(intVar);

				typeStack.push(VT_INT);
				insnIndex += 9;
				break;
			}
			case BC_SLOAD : {
				uint16_t strConstantId = bytecode->getUInt16(insnIndex + 1);
				const string& value = code->constantById(strConstantId);
				sysint_t strPtr = (sysint_t) &value;
				
				GPVar intVar(compiler->newGP());
				compiler->mov(intVar, imm(strPtr));
				compiler->push(intVar);
				compiler->unuse(intVar);

				typeStack.push(VT_STRING);
				insnIndex += 3;
				break;
			}
			case BC_DLOAD0 : {
				pushDouble(0, &compiler);
				
				typeStack.push(VT_DOUBLE);
				insnIndex += 1;
				break;
			}
			case BC_ILOAD0 : {
				compiler->push(imm((sysint_t)0));
				
				typeStack.push(VT_INT);
				insnIndex += 1;
				break;
			}
			case BC_SLOAD0 : {
				const string* value = new string("");
				sysint_t strPtr = (sysint_t) &value;
				compiler->push(imm(strPtr));
				
				typeStack.push(VT_STRING);
				insnIndex += 1;
				break;
			}
			case BC_DLOAD1 : {
				pushDouble(1, &compiler);
				
				typeStack.push(VT_DOUBLE);
				insnIndex += 1;
				break;
			}
			case BC_ILOAD1 : {
				compiler->push(imm((sysint_t)1));
				
				typeStack.push(VT_INT);
				insnIndex += 1;
				break;
			}
			case BC_DLOADM1 : {
				pushDouble(-1, &compiler);
				
				typeStack.push(VT_DOUBLE);
				insnIndex += 1;
				break;
			}
			case BC_ILOADM1 : {
				compiler->push(imm((sysint_t)-1));
				
				typeStack.push(VT_INT);
				insnIndex += 1;
				break;
			}
			case BC_DADD : {
				XMMVar upper = popXMMVar(&compiler);
				XMMVar lower = popXMMVar(&compiler);

				compiler->addsd(upper, lower);
				pushXMMVar(upper, &compiler);
				compiler->unuse(lower);

				// pop two doubles and than push another double
				typeStack.pop();
				insnIndex += 1;
				break;
			}
			case BC_IADD : {
				GPVar upper(compiler->newGP());
				compiler->pop(upper);
				GPVar lower(compiler->newGP());
				compiler->pop(lower);

				compiler->add(upper, lower);
				compiler->push(upper);
				compiler->unuse(upper);
				compiler->unuse(lower);

				typeStack.pop();
				insnIndex += 1;
				break;
			}
			case BC_DSUB : {
				XMMVar upper = popXMMVar(&compiler);
				XMMVar lower = popXMMVar(&compiler);

				compiler->subsd(upper, lower);
				pushXMMVar(upper, &compiler);
				compiler->unuse(lower);

				typeStack.pop();
				insnIndex += 1;
				break;
			}
			case BC_ISUB : {
				GPVar upper(compiler->newGP());
				compiler->pop(upper);
				GPVar lower(compiler->newGP());
				compiler->pop(lower);

				compiler->sub(upper, lower);
				compiler->push(upper);
				compiler->unuse(upper);
				compiler->unuse(lower);

				typeStack.pop();
				insnIndex += 1;
				break;
			}
			case BC_DMUL : {
				XMMVar upper = popXMMVar(&compiler);
				XMMVar lower = popXMMVar(&compiler);

				compiler->mulsd(upper, lower);
				pushXMMVar(upper, &compiler);
				compiler->unuse(lower);

				typeStack.pop();
				insnIndex += 1;
				break;
			}
			case BC_IMUL : {
				GPVar upper(compiler->newGP());
				compiler->pop(upper);
				GPVar lower(compiler->newGP());
				compiler->pop(lower);

				compiler->imul(upper, lower);
				compiler->push(upper);
				compiler->unuse(upper);
				compiler->unuse(lower);

				typeStack.pop();
				insnIndex += 1;
				break;
			}
			case BC_DDIV : {
				XMMVar upper = popXMMVar(&compiler);
				XMMVar lower = popXMMVar(&compiler);

				compiler->divsd(upper, lower);
				pushXMMVar(upper, &compiler);
				compiler->unuse(lower);

				typeStack.pop();
				insnIndex += 1;
				break;
			}
			case BC_IDIV : {
				GPVar upper(compiler->newGP());
				compiler->pop(upper);
				GPVar lower(compiler->newGP());
				compiler->pop(lower);

				// there is no standard idiv
				ECall* ecall = compiler->call((sysint_t)idiv);
				ecall->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<sysint_t, sysint_t, sysint_t>());
				ecall->setArgument(0, upper);
				ecall->setArgument(1, lower);
				ecall->setReturn(upper);

				compiler->push(upper);
				compiler->unuse(upper);
				compiler->unuse(lower);

				typeStack.pop();
				insnIndex += 1;
				break;
			}
			case BC_IMOD : {
				GPVar upper(compiler->newGP());
				compiler->pop(upper);
				GPVar lower(compiler->newGP());
				compiler->pop(lower);

				// there is no standard imod
				ECall* ecall = compiler->call((sysint_t)imod);
				ecall->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<sysint_t, sysint_t, sysint_t>());
				ecall->setArgument(0, upper);
				ecall->setArgument(1, lower);
				ecall->setReturn(upper);

				compiler->push(upper);
				compiler->unuse(upper);
				compiler->unuse(lower);

				typeStack.pop();
				insnIndex += 1;
				break;
			}
			case BC_DNEG : {
				// there is no standard neg for doubles
				pushDouble(0, &compiler);
				XMMVar upper = popXMMVar(&compiler);
				XMMVar lower = popXMMVar(&compiler);

				compiler->subsd(upper, lower);
				pushXMMVar(upper, &compiler);

				compiler->unuse(lower);

				insnIndex += 1;
				break;
			}
			case BC_INEG : {
				GPVar intVar(compiler->newGP());
				compiler->pop(intVar);
				compiler->neg(intVar);
				compiler->push(intVar);
				compiler->unuse(intVar);
				
				insnIndex += 1;
				break;
			}
			case BC_IPRINT : {
				GPVar intVar(compiler->newGP());
				compiler->pop(intVar);

				ECall* ecall = compiler->call((sysint_t)printInt);
				ecall->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, sysint_t>());
				ecall->setArgument(0, intVar);
				compiler->unuse(intVar);

				typeStack.pop();
				insnIndex += 1;
				break;
			}
			case BC_DPRINT : {
				XMMVar doubleVar = popXMMVar(&compiler);

				ECall* ecall = compiler->call((sysint_t)printDouble);
				ecall->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, double>());
				ecall->setArgument(0, doubleVar);
				compiler->unuse(doubleVar);

				typeStack.pop();
				insnIndex += 1;
				break;
			}
			case BC_SPRINT : {
				GPVar strPtr(compiler->newGP());
				compiler->pop(strPtr);

				ECall* ecall = compiler->call((sysint_t)printString);
				ecall->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<void, sysint_t>());
				ecall->setArgument(0, strPtr);
				compiler->unuse(strPtr);

				typeStack.pop();
				insnIndex += 1;
				break;
			}
			case BC_I2D : {
				GPVar intVar(compiler->newGP());
				compiler->pop(intVar);
				XMMVar doubleVar(compiler->newXMM());
				
				// there is no standard i2d
				ECall* ecall = compiler->call((sysint_t)i2d);
				ecall->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<double, sysint_t>());
				ecall->setArgument(0, intVar);				
				ecall->setReturn(doubleVar);
				
				pushXMMVar(doubleVar, &compiler);
				compiler->unuse(intVar);

				typeStack.pop();
				typeStack.push(VT_DOUBLE);
				insnIndex += 1;
				break;
			}
			case BC_D2I : {
				XMMVar doubleVar = popXMMVar(&compiler);
				GPVar intVar(compiler->newGP());
				
				// there is no standard d2i
				ECall* ecall = compiler->call((sysint_t)d2i);
				ecall->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<sysint_t, double>());
				ecall->setArgument(0, doubleVar);				
				ecall->setReturn(intVar);
				
				compiler->push(intVar);
				compiler->unuse(intVar);
				compiler->unuse(doubleVar);
							
				typeStack.pop();
				typeStack.push(VT_INT);
				insnIndex += 1;
				break;
			}
			case BC_S2I : {
				GPVar strPtr(compiler->newGP());
				compiler->pop(strPtr);
				GPVar intVar(compiler->newGP());
				
				// there is no standard s2i
				ECall* ecall = compiler->call((sysint_t)s2i);
				ecall->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder1<sysint_t, sysint_t>());
				ecall->setArgument(0, strPtr);
				ecall->setReturn(intVar);
				
				compiler->push(intVar);
				compiler->unuse(intVar);
				compiler->unuse(strPtr);

				typeStack.pop();
				typeStack.push(VT_INT);
				insnIndex += 1;
				break;
			}			
			case BC_SWAP : {
				XMMVar upperDouble;
				GPVar upperInt;

				VarType upperVarType = typeStack.top();
				typeStack.pop();
				if (upperVarType == VT_DOUBLE) {
					upperDouble = popXMMVar(&compiler);
				}
				else {
					upperInt = compiler->newGP();
					compiler->pop(upperInt);
				}
				
				XMMVar lowerDouble;
				GPVar lowerInt;
				VarType lowerVarType = typeStack.top();
				typeStack.pop();
				if (lowerVarType == VT_DOUBLE) {
					lowerDouble = popXMMVar(&compiler);
				}
				else {
					lowerInt = compiler->newGP();
					compiler->pop(lowerInt);
				}

				// swapping
				if (upperVarType == VT_DOUBLE) {
					pushXMMVar(upperDouble, &compiler);
				}
				else {
					compiler->push(upperInt);
					compiler->unuse(upperInt);
				}
				typeStack.push(upperVarType);

				if (lowerVarType == VT_DOUBLE) {
					pushXMMVar(lowerDouble, &compiler);
				}
				else {
					compiler->push(lowerInt);
					compiler->unuse(lowerInt);
				}
				typeStack.push(lowerVarType);

				insnIndex += 1;
				break;
			}
			case BC_POP : {
				if (typeStack.top() == VT_DOUBLE) {
					XMMVar doubleVar = popXMMVar(&compiler);
					compiler->unuse(doubleVar);
				}
				else {
					GPVar intVar(compiler->newGP());
					compiler->pop(intVar);
					compiler->unuse(intVar);
				}

				typeStack.pop();
				insnIndex += 1;
				break;
			}

			case BC_LOADCTXDVAR : {
				uint16_t ctxId = bytecode->getUInt16(insnIndex + 1);
				uint16_t varId = bytecode->getUInt16(insnIndex + 3);
				Mem varPtr = getVarPtr(&compiler, ctxId, varId, varsStorage_);

				XMMVar newDoubleVar(compiler->newXMM());
				compiler->movq(newDoubleVar, varPtr);
				pushXMMVar(newDoubleVar, &compiler);

				typeStack.push(VT_DOUBLE);
				insnIndex += 5;
				break;
			}
			case BC_LOADCTXIVAR : {
				uint16_t ctxId = bytecode->getUInt16(insnIndex + 1);
				uint16_t varId = bytecode->getUInt16(insnIndex + 3);
				Mem varPtr = getVarPtr(&compiler, ctxId, varId, varsStorage_);

				compiler->push(varPtr);

				typeStack.push(VT_INT);
				insnIndex += 5;
				break;
			}
			case BC_LOADCTXSVAR : {
				uint16_t ctxId = bytecode->getUInt16(insnIndex + 1);
				uint16_t varId = bytecode->getUInt16(insnIndex + 3);
				Mem varPtr = getVarPtr(&compiler, ctxId, varId, varsStorage_);

				compiler->push(varPtr);

				typeStack.push(VT_STRING);
				insnIndex += 5;
				break;
			}
			case BC_STORECTXDVAR : {
				uint16_t ctxId = bytecode->getUInt16(insnIndex + 1);
				uint16_t varId = bytecode->getUInt16(insnIndex + 3);
				Mem varPtr = getVarPtr(&compiler, ctxId, varId, varsStorage_);

				// it is function argument
				if (curParameter > 0) {
					XMMVar argVar(compiler->argXMM(--curParameter));
					compiler->movq(varPtr, argVar);
					compiler->unuse(argVar);
				}
				else {
					XMMVar doubleVarTOS = popXMMVar(&compiler);
					compiler->movq(varPtr, doubleVarTOS);
					compiler->unuse(doubleVarTOS);
					typeStack.pop();
				}
				insnIndex += 5;
				break;
			}
			case BC_STORECTXIVAR : {
				uint16_t ctxId = bytecode->getUInt16(insnIndex + 1);
				uint16_t varId = bytecode->getUInt16(insnIndex + 3);
				Mem varPtr = getVarPtr(&compiler, ctxId, varId, varsStorage_);

				// it is function argument
				if (curParameter > 0) {
					GPVar argVar(compiler->argGP(--curParameter));
					compiler->mov(varPtr, argVar);
					compiler->unuse(argVar);
				}
				else
				{
					compiler->pop(varPtr);
					typeStack.pop();
				}

				insnIndex += 5;
				break;
			}
			case BC_STORECTXSVAR : {
				uint16_t ctxId = bytecode->getUInt16(insnIndex + 1);
				uint16_t varId = bytecode->getUInt16(insnIndex + 3);
				Mem varPtr = getVarPtr(&compiler, ctxId, varId, varsStorage_);

				if (curParameter > 0) {
					GPVar argVar(compiler->argGP(--curParameter));
					compiler->mov(varPtr, argVar);
					compiler->unuse(argVar);
				}
				else {
					compiler->pop(varPtr);
					typeStack.pop();
				}

				insnIndex += 5;
				break;
			}
			case BC_DCMP : {
				XMMVar upper = popXMMVar(&compiler);
				XMMVar lower = popXMMVar(&compiler);
				GPVar result(compiler->newGP());

				ECall* ecall = compiler->call((sysint_t)dcmp);
				ecall->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<sysint_t, double, double>());
				ecall->setArgument(0, upper);
				ecall->setArgument(1, lower);
				ecall->setReturn(result);

				compiler->push(result);
				compiler->unuse(upper);
				compiler->unuse(lower);
				compiler->unuse(result);

				typeStack.pop();
				typeStack.pop();
				typeStack.push(VT_INT);
				insnIndex += 1;
				break;
			}
			case BC_ICMP : {
				GPVar upper(compiler->newGP());
				compiler->pop(upper);
				GPVar lower(compiler->newGP());
				compiler->pop(lower);
				GPVar result(compiler->newGP());

				ECall* ecall = compiler->call((sysint_t)icmp);
				ecall->setPrototype(CALL_CONV_DEFAULT, FunctionBuilder2<sysint_t, sysint_t, sysint_t>());
				ecall->setArgument(0, upper);
				ecall->setArgument(1, lower);
				ecall->setReturn(result);

				compiler->push(result);
				compiler->unuse(upper);
				compiler->unuse(lower);
				compiler->unuse(result);

				typeStack.pop();
				insnIndex += 1;
				break;
			}
			case BC_JA : {
				int16_t offset = bytecode->getInt16(insnIndex + 1);
				uint32_t jumpInsn = insnIndex + 1 + offset;

				AsmJit::Label* label = labelMap[jumpInsn];
				compiler->jmp(*label);

				insnIndex += 3;
				break;
			}
			case BC_IFICMPNE :
			case BC_IFICMPE  :
			case BC_IFICMPG  :
			case BC_IFICMPGE :
			case BC_IFICMPL  :
			case BC_IFICMPLE : {

				handleIFICMP(&compiler, labelMap, bytecode, insnIndex, curInstruction);

				typeStack.pop();
				typeStack.pop();
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
				BytecodeFunction* bytecodeFunc = (BytecodeFunction*)code->functionById(funcId);
				if (bytecodeFunc == 0) {
					freeLabelMap(&labelMap);
					return new Status("Function not found", insnIndex);
				}

				// check if code not generated for this function
				if (funcStorage_[funcId] == 0) {
					funcStorage_[funcId] = (void*)1;
					Status* status = generateFunction(code, bytecodeFunc, &funcStorage_[funcId]);
					if (status != NULL) {
						freeLabelMap(&labelMap);
						return status;
					}
				}

				// loading arguments from stack (reverse order)
				BaseVar** args = new BaseVar*[bytecodeFunc->parametersNumber()];
				for (uint16_t i = bytecodeFunc->parametersNumber(); i != 0; --i) {
					if (bytecodeFunc->parameterType(i - 1) == VT_DOUBLE) {
						args[i - 1] = new XMMVar(popXMMVar(&compiler));
					}
					else {
						GPVar intVar(compiler->newGP());
						compiler->pop(intVar);
						args[i - 1] = new GPVar(intVar);
					}
					typeStack.pop();
				}

				GPVar funcAddr(compiler->newGP());
				compiler->mov(funcAddr, imm((sysint_t) (funcStorage_ + funcId) ));

				// changing called function stack pointer
				{
				GPVar baseStackPtr(compiler->newGP());
				GPVar baseStackAddr(compiler->newGP());
				compiler->mov(baseStackAddr, imm( (sysint_t) &varsStorage_));
				compiler->mov(baseStackPtr, qword_ptr(baseStackAddr));
				Mem curStackPtr = qword_ptr(baseStackPtr, funcId * sizeof(func_vars_t));
				compiler->add(curStackPtr, imm( (sysint_t) (funcVarsCount_[funcId] * sizeof(var_t)) ));
				compiler->unuse(baseStackPtr);
				compiler->unuse(baseStackAddr);
				}

				compiler->comment("calling other function");
				ECall* ecall = compiler->call(ptr(funcAddr));
				compiler->unuse(funcAddr);

				FunctionBuilderX functionBuilderX = fillFunctionBuilderX(bytecodeFunc);
				ecall->setPrototype(CALL_CONV_DEFAULT, functionBuilderX);

				// setting arguments to function (normal order)
				for (uint16_t i = 0; i != bytecodeFunc->parametersNumber(); ++i) {
					if (bytecodeFunc->parameterType(i) == VT_DOUBLE) {
						ecall->setArgument(i, *((XMMVar*)args[i]));
						compiler->unuse(*((XMMVar*)args[i]));
						delete args[i];
					}
					else {
						ecall->setArgument(i, *((GPVar*)args[i]));
						compiler->unuse(*((GPVar*)args[i]));
						delete args[i];
					}
				}
				delete [] args;

				// setting return value
				if (bytecodeFunc->returnType() == VT_DOUBLE) {
					XMMVar doubleVar(compiler->newXMM());
					ecall->setReturn(doubleVar);
					pushXMMVar(doubleVar, &compiler);
					typeStack.push(VT_DOUBLE);
				}
				else if (bytecodeFunc->returnType() != VT_VOID) {
					GPVar intVar(compiler->newGP());
					ecall->setReturn(intVar);
					compiler->push(intVar);
					compiler->unuse(intVar);
					typeStack.push(VT_INT);
				}

				compiler->comment("returning from other function");

				// changing called function stack pointer
				{
				GPVar baseStackPtr(compiler->newGP());
				GPVar baseStackAddr(compiler->newGP());
				compiler->mov(baseStackAddr, imm( (sysint_t) &varsStorage_));
				compiler->mov(baseStackPtr, qword_ptr(baseStackAddr));
				Mem curStackPtr = qword_ptr(baseStackPtr, funcId * sizeof(func_vars_t));
				compiler->sub(curStackPtr, imm( (sysint_t) (funcVarsCount_[funcId] * sizeof(var_t)) ));
				compiler->unuse(baseStackPtr);
				compiler->unuse(baseStackAddr);
				}

				insnIndex += 3;
				break;
			}
			case BC_CALLNATIVE : {
				return new Status("Native functions not supported yet", insnIndex);
			}
			case BC_RETURN : {
				if (bytecodeFunction->returnType() != VT_VOID ) {
					if (typeStack.empty()) {
						freeLabelMap(&labelMap);
						return new Status("Stack is empty, but function is not VOID!");
					}

					if (typeStack.top() == VT_DOUBLE) {
						XMMVar doubleVar = popXMMVar(&compiler);
						compiler->ret(doubleVar);
						compiler->unuse(doubleVar);
					}
					else
					{
						GPVar intVar(compiler->newGP());
						compiler->pop(intVar);
						compiler->ret(intVar);
						compiler->unuse(intVar);
					}
				}
				else {
					compiler->ret();
				}

				insnIndex += 1;
				break;
			}
			default : {
				freeLabelMap(&labelMap);
				return new Status("Not handled instruction", insnIndex);
			}
		}
	}

	compiler->endFunction();
	(*function) = compiler->make();

	freeLabelMap(&labelMap);
	return NULL;
}
//// END OF MAIN FUNCTION to translate BYTECODE into MACHCODE /////

// functions for prepare MAIN translation //
Status* MyJitTranslator::generate(Code* code, MyMachCodeImpl* *machCode) {
	// preparing variables storage
	funcVarsCount_ = *((MyCode*)code)->getFuncVarsCount();

	uint16_t funcNumber = funcVarsCount_.size();
	varsStorage_ = new func_vars_t[funcNumber];
	// special case
	varsStorage_[0] = new var_t[funcVarsCount_[0]];
	// all other functions
	for (uint16_t i = 1; i != funcNumber; ++i) {
		varsStorage_[i] = new var_t[funcVarsCount_[i] * MAX_RECURSIVE_CALLS];
		varsStorage_[i] = (varsStorage_[i]) - funcVarsCount_[i];
	}

	funcStorage_ = new func_t[funcNumber];
	memset(funcStorage_, 0, funcNumber * sizeof(func_t));

	BytecodeFunction* pseudo_function = (BytecodeFunction*) code->functionById(0);
	Status* generationStatus = generateFunction(code, pseudo_function, &funcStorage_[0]);
	if (generationStatus == NULL || generationStatus->isOk())
		(*machCode)->setCode(funcStorage_[0]);

	return generationStatus;
}

Status* MyJitTranslator::translateMachCode(const string& program, MyMachCodeImpl* *result) {
	Parser* parser = new Parser();
	// Build an AST
	Status* status = parser->parseProgram(program);
	if (status == NULL) {
		// Generate Bytecode
		Code* code = new MyCode();
		BytecodeFunction* pseudo_function = new BytecodeFunction(parser->top());
		code->addFunction(pseudo_function);
		Bytecoder* visitor = new Bytecoder(code);
		parser->top()->node()->visit(visitor);
		pseudo_function->bytecode()->add(BC_STOP);
		delete visitor;

		// Generating MachCodeImpl
		MyMachCodeImpl* machCode = new MyMachCodeImpl();
		Status* generationStatus = generate(code, &machCode);
		delete code;
		if (generationStatus != NULL) {
			delete parser;
			delete status;
			return generationStatus;
		}
		*result = machCode;
	}
	delete parser;
	return status;
}

Status* MyJitTranslator::translate(const string& program, Code* *result) {
	MyMachCodeImpl* code = 0;
    Status* status = 0;

    status = translateMachCode(program, &code);
    if (status != 0) {
        assert(code == 0);
        *result = 0;
        return status;
    }

    //code->disassemble();
    assert(code);
    *result = code;
    return NULL;
}
// end of functions for prepare MAIN translation //

}
