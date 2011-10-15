#pragma once
#include "mathvm.h"
#include "Stack.h"
#include "VarTable.h"

#include <vector>
#include <string>

namespace mathvm  {
	
static int instrunction_size[] = {
#define INSTRUCTION_SIZE(b, d, l) l,
    FOR_BYTECODES(INSTRUCTION_SIZE)
#undef INSTRUCTION_SIZE
};

enum StackVarType {
	SV_INT,
	SV_DOUBLE,
	SV_STRING,
	SV_INVALID
};

class StackVar {

private:
	StackVarType _type;
	union{
		int64_t  i;
		double   d;
		char*    s;
		uint16_t ref;
    } _value;

public:
	StackVar() : _type(SV_INVALID){}
	
	StackVar(StackVarType varType): _type(varType) {}
	
	StackVar(char* str): _type(SV_STRING) {
		_value.s = str;
	}
	
	StackVar(uint64_t i): _type(SV_INT) {
		_value.i = i;
	}
	
	StackVar(double d): _type(SV_DOUBLE) {
		_value.d = d;
	}
	
	uint64_t getInt() {
		assert(_type == SV_INT && "Can`t getInt: Variable type is not integer");
		return _value.i;
	}
	
	double getDouble() {
		assert(_type == SV_DOUBLE && "Can`t getDouble: Variable type is not double");
		return _value.d;
	}
	
	char* getString() {
		assert(_type == SV_STRING && "Can`t getString: Variable type is not string");
		return _value.s;
	}
	StackVarType getType() {
		return _type;
	}
};

class CodeInterpreter :  public Code {
	
private:
	Stack _stack;
	uint64_t icmp(uint64_t op1, uint64_t op2);
	uint64_t dcmp(double op1, double op2);
public:
	CodeInterpreter(void);
	~CodeInterpreter(void);
	Status* execute(std::vector<Var*> vars);
};

}


