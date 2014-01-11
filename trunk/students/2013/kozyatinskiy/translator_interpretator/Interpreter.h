#pragma once

#include <mathvm.h>
using namespace mathvm;

#include <cstring>

#include "VarsUtil.h"
#include "CompilerVisitor.h"

template<typename T>
VarType type();

/*
roadmap:
1. esp,eip to int*
2. instruction jit
*/

template<> inline VarType type<int64_t>(){ return VT_INT; }
template<> inline VarType type<int64_t*>(){ return VT_STRING; }
template<> inline VarType type<double>(){ return VT_DOUBLE; }
template<> inline VarType type<double*>(){ return VT_STRING; }
template<> inline VarType type<char*>(){ return VT_STRING; }

class Interpreter
{
public:
	Interpreter();
	~Interpreter();

	void execute(const vector<Bytecode_>& bytecodes, const vector<string>& literals);

private:
	// fst - esp, snd - function ptr
	typedef double  (*DoubleCall)(void*, void*);
	typedef int64_t (*IntCall)(void*, void*);
	typedef char*   (*StringCall)(void*, void*);
	typedef void    (*VoidCall)(void*, void*);

	DoubleCall doubleCallWrapper;
	IntCall    intCallWrapper;
	StringCall stringCallWrapper;
	VoidCall   voidCallWrapper;

	void call(int id);

	vector<Bytecode_> bytecodes_;
	vector<string>   literals_;
	vector<pair<void*, VarType> > resolved_;
	int8_t* ebp_;
	int8_t* esp_;
	uint8_t* eip_;

	vector<int16_t> functions_;

	double deax, dvar1, dvar2, dvar3;
	int64_t ieax, ivar3;
	char *seax, *svar1, *svar2, *svar3;

	int8_t* stack_;

	template<typename T>
	T* popValue()
	{
		int8_t* tmp = esp_;
		esp_ += sizeof(T);
		return reinterpret_cast<T*>(tmp);
	}

	template <typename T>
	void pushValue(T val)
	{
		esp_ -= sizeof(T);
		memcpy(esp_, &val, sizeof(T));
	}
};

