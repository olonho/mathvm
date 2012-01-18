#pragma once

#include <stdint.h>
#include "mathvm.h"
#include "ast.h"
#include "FuncStackFrame.h"
#include "DataStack.h"

class BytecodeInterpreter
{
	mathvm::Code *_code;
	mathvm::Bytecode *_bytecode;
	uint32_t _insnPtr;
	FuncStackFrame* _stackFrame;
	DataStack* _dataStack;
	bool _shouldReturn;
	
	mathvm::Status processInsn(mathvm::Instruction);

	uint16_t getNextUInt16();
	int16_t getOffset();
	int64_t getTOSInt();
	double getTOSDouble();
	const char* getTOSStr();
	int64_t cmp(int64_t val1, int64_t val2);
	int64_t cmp(double val1, double val2);

	void initFuncParameters();
	
public:
	BytecodeInterpreter(mathvm::Code *code);
	~BytecodeInterpreter(void);
	void jump(int32_t offset);
	mathvm::Status* call(uint16_t id);
};


struct InterpretationException {
  
  InterpretationException(std::string const& message) : _message(message) {}
  
  virtual std::string what() const { return _message; }
  
private:
  std::string _message;
};

