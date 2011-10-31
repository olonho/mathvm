#include "ExecutableCode.h"
#include "FreeVarsFunction.h"

using namespace mathvm;

ExecutableCode::ExecutableCode() : _dataStack(new Value[DATA_STACK_SIZE]), _callStack(new StackFrameInfo[CALL_STACK_SIZE])
{
}


ExecutableCode::~ExecutableCode()
{
	delete[] _dataStack;
	delete[] _callStack;
}

mathvm::Status * ExecutableCode::execute( std::vector<mathvm::Var*> &vars )
{	
	FreeVarsFunction *mainFunc = static_cast<FreeVarsFunction *>(functionById(0));
	NewByteCode *mainBytecode = mainFunc->bytecode();
	_code.insn = (uint8_t *)mainBytecode->getData();
	Value *tos = _dataStack + mainFunc->localsNumber() + 1;
	Value *varsPtr = _dataStack;	
	StackFrameInfo *callInfo = _callStack;
	Value tempVal;
	int boolResult;
	bool executing = true;
	int i = 0;
	while (executing) {		
		switch (*_code.insn++) {	
			case BC_INVALID	: throwError("Invalid instruction"); break;			// "Invalid instruction.", 1)                          
			case BC_DLOAD : tos->doubleVal = *_code.doublePtr++; tos++; break;	// "Load double on TOS, inlined into insn stream.", 9)   
			case BC_ILOAD :	tos->intVal = *_code.intPtr++; tos++; break;		// "Load int on TOS, inlined into insn stream.", 9)      
			case BC_SLOAD : tos->stringVal = constantById(*_code.strId++).c_str(); tos++; break; // "Load string reference on TOS, next two bytes - constant id.", 3)   
			case BC_DLOAD0 : tos->doubleVal = 0.0; tos++; break;				// "Load double 0 on TOS.", 1)                          
			case BC_ILOAD0 : tos->intVal = 0; tos++; break;						//"Load int 0 on TOS.", 1)                             
			case BC_SLOAD0 : tos->stringVal = ""; tos++; break;					// "Load empty string on TOS.", 1)                      
			case BC_DLOAD1 : tos->doubleVal = 1.0; tos++; break;				// "Load double 1 on TOS.", 1)                          
			case BC_ILOAD1 : tos->intVal = 1; tos++; break;						// "Load int 1 on TOS.", 1)                             
			case BC_DLOADM1 : tos->doubleVal = -1.0; tos++; break;				// "Load double -1 on TOS.", 1)                        
			case BC_ILOADM1 : tos->intVal = -1; tos++; break;					// "Load int -1 on TOS.", 1)                           
			case BC_DADD : tempVal.doubleVal = (tos - 1)->doubleVal + (tos - 2)->doubleVal; (tos - 2)->doubleVal = tempVal.doubleVal; tos--; break;  // "Add 2 doubles on TOS, push value back.", 1)           
			case BC_IADD : tempVal.intVal = (tos - 1)->intVal + (tos - 2)->intVal; (tos - 2)->intVal = tempVal.intVal; tos--; break;// "Add 2 ints on TOS, push value back.", 1)              
			case BC_DSUB : tempVal.doubleVal = (tos - 1)->doubleVal - (tos - 2)->doubleVal; (tos - 2)->doubleVal = tempVal.doubleVal; tos--; break; // "Subtract 2 doubles on TOS (lower from upper), push value back.", 1) 
			case BC_ISUB : tempVal.intVal = (tos - 1)->intVal - (tos - 2)->intVal; (tos - 2)->intVal = tempVal.intVal; tos--; break;// "Subtract 2 ints on TOS (lower from upper), push value back.", 1) 
			case BC_DMUL : tempVal.doubleVal = (tos - 1)->doubleVal * (tos - 2)->doubleVal; (tos - 2)->doubleVal = tempVal.doubleVal; tos--; break;// "Multiply 2 doubles on TOS, push value back.", 1)      
			case BC_IMUL : tempVal.intVal = (tos - 1)->intVal * (tos - 2)->intVal; (tos - 2)->intVal = tempVal.intVal; tos--; break;// "Multiply 2 ints on TOS, push value back.", 1)         
			case BC_DDIV : tempVal.doubleVal = (tos - 1)->doubleVal / (tos - 2)->doubleVal; (tos - 2)->doubleVal = tempVal.doubleVal; tos--; break;// "Divide 2 doubles on TOS (upper to lower), push value back.", 1) 
			case BC_IDIV : tempVal.intVal = (tos - 1)->intVal / (tos - 2)->intVal; (tos - 2)->intVal = tempVal.intVal; tos--; break;// "Divide 2 ints on TOS (upper to lower), push value back.", 1) 
			case BC_DNEG : (tos - 1)->doubleVal  *= -1.0; break;				// "Negate double on TOS.", 1)                            
			case BC_INEG : (tos - 1)->intVal *= -1; break;						// "Negate int on TOS.", 1)                               
			case BC_IPRINT : printf("%lld", (tos - 1)->intVal); tos--; break;		// "Pop and print integer TOS.", 1)                     
			case BC_DPRINT : printf("%f", (tos - 1)->doubleVal); tos--; break;	// "Pop and print double TOS.", 1)                      
			case BC_SPRINT : printf("%s", (tos - 1)->stringVal); tos--; break;	// "Pop and print string TOS.", 1)                      
			case BC_I2D : (tos - 1)->doubleVal = (tos - 1)->intVal; break;		//  "Convert int on TOS to double.", 1)                    
			case BC_D2I : (tos - 1)->intVal = (tos - 1)->doubleVal; break;		//  "Convert double on TOS to int.", 1)                    
			case BC_SWAP : tempVal = *(tos - 1); *(tos - 1) = *(tos - 2); *(tos - 2) = tempVal; break;	// "Swap 2 topmost values.", 1)                           
			case BC_POP : tos--; break;											// "Remove topmost value.", 1)                             
			//BC_LOADDVAR0// "Load double from variable 0, push on TOS.", 1)   
			//BC_LOADDVAR1// "Load double from variable 1, push on TOS.", 1)   
			//BC_LOADDVAR2// "Load double from variable 2, push on TOS.", 1)   
			//BC_LOADDVAR3// "Load double from variable 3, push on TOS.", 1)   
			//BC_LOADIVAR0// "Load int from variable 0, push on TOS.", 1)      
			//BC_LOADIVAR1// "Load int from variable 1, push on TOS.", 1)      
			//BC_LOADIVAR2// "Load int from variable 2, push on TOS.", 1)      
			//BC_LOADIVAR3// "Load int from variable 3, push on TOS.", 1)      
			//BC_LOADSVAR0// "Load string from variable 0, push on TOS.", 1)   
			//BC_LOADSVAR1// "Load string from variable 1, push on TOS.", 1)   
			//BC_LOADSVAR2// "Load string from variable 2, push on TOS.", 1)   
			//BC_LOADSVAR3// "Load string from variable 3, push on TOS.", 1)   
			//BC_STOREDVAR0// "Pop TOS and store to double variable 0.", 1)    
			//BC_STOREDVAR1// "Pop TOS and store to double variable 1.", 1)    
			//BC_STOREDVAR2// "Pop TOS and store to double variable 0.", 1)    
			//BC_STOREDVAR3// "Pop TOS and store to double variable 3.", 1)    
			//BC_STOREIVAR0// "Pop TOS and store to int variable 0.", 1)       
			//BC_STOREIVAR1// "Pop TOS and store to int variable 1.", 1)       
			//BC_STOREIVAR2// "Pop TOS and store to int variable 0.", 1)       
			//BC_STOREIVAR3// "Pop TOS and store to int variable 3.", 1)       
			//BC_STORESVAR0// "Pop TOS and store to string variable 0.", 1)    
			//BC_STORESVAR1// "Pop TOS and store to string variable 1.", 1)    
			//BC_STORESVAR2// "Pop TOS and store to string variable 0.", 1)    
			//BC_STORESVAR3// "Pop TOS and store to string variable 3.", 1)    
			case BC_LOADDVAR : tos->doubleVal = (varsPtr + *_code.varId++)->doubleVal; tos++; break;	// "Load double from variable, whose 2-byte is id inlined to insn stream, push on TOS.", 3) 
			case BC_LOADIVAR : tos->intVal = (varsPtr + *_code.varId++)->intVal; tos++; break;			// "Load int from variable, whose 2-byte id is inlined to insn stream, push on TOS.", 3) 
			case BC_LOADSVAR : tos->stringVal = (varsPtr + *_code.strId++)->stringVal; tos++; break;	// "Load string from variable, whose 2-byte id is inlined to insn stream, push on TOS.", 3) 
			case BC_STOREDVAR : (varsPtr + *_code.varId++)->doubleVal = (tos - 1)->doubleVal; tos--; break;	// "Pop TOS and store to double variable, whose 2-byte id is inlined to insn stream.", 3) 
			case BC_STOREIVAR : (varsPtr + *_code.varId++)->intVal = (tos - 1)->intVal; tos--; break;			// "Pop TOS and store to int variable, whose 2-byte id is inlined to insn stream.", 3) 
			case BC_STORESVAR : (varsPtr + *_code.varId++)->stringVal = (tos - 1)->stringVal; tos--; break;// "Pop TOS and store to string variable, whose 2-byte id is inlined to insn stream.", 3) 
			//BC_LOADCTXDVAR// "Load double from variable, whose 2-byte context and 2-byte id inlined to insn stream, push on TOS.", 5) 
			//BC_LOADCTXIVAR// "Load int from variable, whose 2-byte context and 2-byte id is inlined to insn stream, push on TOS.", 5) 
			//BC_LOADCTXSVAR// "Load string from variable, whose 2-byte context and 2-byte id is inlined to insn stream, push on TOS.", 5) 
			//BC_STORECTXDVAR// "Pop TOS and store to double variable, whose 2-byte context and 2-byte id is inlined to insn stream.", 5) 
			//BC_STORECTXIVAR// "Pop TOS and store to int variable, whose 2-byte context and 2-byte id is inlined to insn stream.", 5) 
			//BC_STORECTXSVAR// "Pop TOS and store to string variable, whose 2-byte context and 2-byte id is inlined to insn stream.", 5) 
			case BC_DCMP : {			// "Compare 2 topmost doubles, pushing libc-stryle comparator value cmp(upper, lower) as integer.", 1) 				
					if ((tos - 1)->doubleVal < (tos - 2)->doubleVal) {
						boolResult = -1;
					}
					else if ((tos - 1)->doubleVal > (tos - 2)->doubleVal) {
						boolResult = 1;
					}
					else boolResult = 0;
					(tos - 2)->intVal = boolResult;
					tos--;
					break;
				}
			case BC_ICMP : {			// "Compare 2 topmost ints, pushing libc-stryle comparator value cmp(upper, lower) as integer.", 1) 					
					if ((tos - 1)->intVal < (tos - 2)->intVal) {
						boolResult = -1;
					}
					else if ((tos - 1)->intVal > (tos - 2)->intVal) {
						boolResult = 1;
					}
					else boolResult = 0;
					(tos - 2)->intVal = boolResult;
					tos--;
					break;
				}
			case BC_JA : _code.insn += *_code.jump; break;	// "Jump always, next two bytes - signed offset of jump destination.", 3) 
			case BC_IFICMPNE : // "Compare two topmost integers and jump if upper != lower, next two bytes - signed offset of jump destination.", 3) 
								if ((tos - 1)->intVal != (tos - 2)->intVal) 
								   _code.insn += *_code.jump; 
								else 
								   _code.jump++;
								tos -= 2; break;	
			case BC_IFICMPE :  // "Compare two topmost integers and jump if upper == lower, next two bytes - signed offset of jump destination.", 3) 
								if ((tos - 1)->intVal == (tos - 2)->intVal) 
								   _code.insn += *_code.jump; 
								else 
								   _code.jump++;
								tos -= 2; break;	
			case BC_IFICMPG :	// "Compare two topmost integers and jump if upper > lower, next two bytes - signed offset of jump destination.", 3) 
								if ((tos - 1)->intVal > (tos - 2)->intVal) 
								   _code.insn += *_code.jump; 
								else 
								   _code.jump++;
								tos -= 2; break;	
			case BC_IFICMPGE :	// "Compare two topmost integers and jump if upper >= lower, next two bytes - signed offset of jump destination.", 3) 
								if ((tos - 1)->intVal >= (tos - 2)->intVal) 
								   _code.insn += *_code.jump; 
								else 
								   _code.jump++;
								tos -= 2; break;	
			case BC_IFICMPL :  // "Compare two topmost integers and jump if upper < lower, next two bytes - signed offset of jump destination.", 3) 
								if ((tos - 1)->intVal < (tos - 2)->intVal) 
								   _code.insn += *_code.jump; 
								else 
								   _code.jump++;
								tos -= 2; break;	
			case BC_IFICMPLE :	// "Compare two topmost integers and jump if upper <= lower, next two bytes - signed offset of jump destination.", 3)
								if ((tos - 1)->intVal <= (tos - 2)->intVal) 
								   _code.insn += *_code.jump; 
								else 
								   _code.jump++;
								tos -= 2; break;	
			//case BC_DUMP : // "Dump value on TOS, without removing it.", 1)        
			case BC_STOP : executing = false; break;			// "Stop execution.", 1)                                  
			case BC_CALL : {									// "Call function, next two bytes - unsigned function id.", 3) 								
					FreeVarsFunction *func = static_cast<FreeVarsFunction *>(functionById(*_code.funcId++));
					NewByteCode *newByteCode = func->bytecode();
					callInfo->codePtr = _code;
					callInfo->stackPointer = tos - func->parametersNumber();
					callInfo->varPointer = varsPtr;
					callInfo++;
					_code.insn = (uint8_t *)newByteCode->getData();				
					varsPtr = tos - (func->parametersNumber() + func->freeVars().size() + 1);
					tos += func->localsNumber();
					break;
				}
			case BC_RETURN : {									// "Return to call location", 1) 
					callInfo--;
					_code = callInfo->codePtr;
					tos = callInfo->stackPointer;
					varsPtr = callInfo->varPointer;
					break;
				}
			//BC_BREAK// "Breakpoint for the debugger.", 1)
			default: throwError("Unimplemented instruction");
		}			
		i++;
	}

	return NULL;
}
