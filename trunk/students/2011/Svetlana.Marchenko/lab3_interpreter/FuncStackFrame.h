#pragma once

#include <stdint.h>
#include <string.h>

#include "mathvm.h"
#include "ast.h"
#include "DataStack.h"

struct FuncFrame {
	uint16_t _id;
	uint32_t _insnPtr;
	FuncFrame* _prevCall;
	FuncFrame* _prevFunc;
	uint16_t _countParams;
	uint16_t _countVars;
	char* _vars;
	
	FuncFrame() : 
		_id(0), 
		_insnPtr(0),
		_prevCall(0),
		_prevFunc(0),
		_countParams(0),
		_countVars(0)
	{
		_vars = (char*)&_vars + sizeof(_vars); //_vars points to the next cell	
	}

	size_t size() {
		return sizeof(struct FuncFrame) + _countVars*sizeof(struct StackValue);
	}

	void setVar(uint16_t id, StackValue value) {
		StackValue *var = (StackValue *)(_vars+sizeof(struct StackValue)*(id-1));	
		memcpy(var, &value, sizeof(struct StackValue));
	}

	StackValue getVar(uint16_t id) {
		StackValue *var = (StackValue *)(_vars+sizeof(struct StackValue)*(id-1));
		return *var;
	}
};

class FuncStackFrame
{
	uint64_t _stackSize;
	FuncFrame* _stack;
	FuncFrame* _topStack;
	bool _isEmpty;

	FuncFrame* firstFrameWithId(uint16_t id);

public:
	FuncStackFrame(uint64_t stackSize = 2048*100);
	~FuncStackFrame(void);
	StackValue getVar(uint16_t ctx, uint16_t id);
	StackValue getVar(uint16_t id);
	void updateVar(uint16_t ctx, uint16_t id, StackValue val);
	void updateVar(uint16_t id, StackValue val);
	FuncFrame* addFrame(mathvm::TranslatedFunction *func);
	void removeTop();
	bool isEmpty();
	FuncFrame* getCurFrame();
};

struct ExecutionException {
  
  ExecutionException(std::string const& message) : _message(message) {}
  
  virtual std::string what() const { return _message; }
  
private:
  std::string _message;
};