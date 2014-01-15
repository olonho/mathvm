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
1. call between jit and interpreter
*/

/*
Two different contexts:
1. Interpreter context - fake return registers, ebp, esp, eip
2. Compiled context - real return, rsp, rbp, rip

When i change context from Interpreter to Compiled:
1. Save Interpreter context
2. r15 = eip, rsp = esp, rbp = rsp, rip = next compiled instruction
3. Restore Interpreter context
esp = rsp, ebp = rbp, eip = r15

Compiled to Interpreter:

*/

class MyCompiler;

template<> inline VarType type<int64_t>(){ return VT_INT; }
template<> inline VarType type<int64_t*>(){ return VT_STRING; }
template<> inline VarType type<double>(){ return VT_DOUBLE; }
template<> inline VarType type<double*>(){ return VT_STRING; }
template<> inline VarType type<char*>(){ return VT_STRING; }

class Interpreter
{
	friend class MyCompiler;
public:
	Interpreter();
	~Interpreter();

	void execute(const vector<pair<VarType, Bytecode_> >& bytecodes, const vector<string>& literals);

	const vector<int>& callsCount() const { return callsCount_; }

	void setCompiler(MyCompiler* _compiler);

	static void callHelper(Interpreter* interp, int id, int8_t* esp, int8_t* ebp);
private:
	void call(int id, int8_t* esp, int8_t* ebp, bool fromNative = false);
	bool tryCompileAndRun(int16_t id);

	// fst - esp, snd - ebp, thd - function ptr
	typedef double  (*DoubleNCall)(void*, void*, void*);
	typedef int64_t (*IntNCall)(void*, void*, void*);
	typedef char*   (*StringNCall)(void*, void*, void*);
	typedef void    (*VoidNCall)(void*, void*, void*);

	// call wrappers
	typedef double  (*DoubleCall)(void*, void*, void*, void*);
	typedef int64_t (*IntCall)(void*, void*, void*, void*);
	typedef char*   (*StringCall)(void*, void*, void*, void*);
	typedef void    (*VoidCall)(void*, void*, void*, void*);

	typedef int8_t* (*InstWrapper)(int8_t*);

	DoubleNCall doubleCallWrapper;
	IntNCall    intCallWrapper;
	StringNCall stringCallWrapper;
	VoidNCall   voidCallWrapper;

	DoubleCall doubleSCallWrapper;
	IntCall    intSCallWrapper;
	StringCall stringSCallWrapper;
	VoidCall   voidSCallWrapper;

	InstWrapper icmpWrapper;
	InstWrapper iload0Wrapper;

	vector<pair<VarType, Bytecode_> > bytecodes_;
	vector<string>    literals_;
	vector<pair<void*, VarType> > resolved_;
	vector<int>       callsCount_;
	int8_t* ebp_;
	int8_t* esp_;
	const uint8_t* eip_;

	MyCompiler* compiler_;

	vector<int16_t> functions_;

	double deax, dvar1, dvar2, dvar3;
	int64_t ieax, ivar3;
	char *seax, *svar1, *svar2, *svar3;

	int8_t* stack_;

	template<typename T>
	inline T* popValue()
	{
		int8_t* tmp = esp_;
		esp_ += sizeof(T);
		return reinterpret_cast<T*>(tmp);
	}

	template <typename T>
	inline void pushValue(T val)
	{
		esp_ -= sizeof(T);
		memcpy(esp_, &val, sizeof(T));
	}
};

