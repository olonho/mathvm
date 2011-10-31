#pragma once
#include "mathvm.h"
#include "Exceptions.h"

class ExecutableCode : public mathvm::Code
{
private:
	static const size_t DATA_STACK_SIZE = 1024 * 1024 * 16;
	static const size_t CALL_STACK_SIZE = 1024 * 1024 * 1;
	union CodePointer {
		uint8_t *insn;
		int16_t *jump;
		uint16_t *varId;
		uint16_t *funcId;
		uint16_t *strId;
		int64_t *intPtr;
		double *doublePtr;
	} _code;

	union Value {
		int64_t intVal;
		double doubleVal;
		const char *stringVal;
	} *_dataStack;

	struct StackFrameInfo {
		CodePointer codePtr;
		Value *varPointer;
		Value *stackPointer;
	} *_callStack;

protected:
	void throwError(std::string what) {
		throw Exception(what);
	}

public:
	ExecutableCode(void);
	~ExecutableCode(void);	
	virtual mathvm::Status * execute(std::vector<mathvm::Var*> &vars);
};

