#pragma once

#include <mathvm.h>
using namespace mathvm;

#include <cstring>

#include "VarsUtil.h"
#include "CompilerVisitor.h"

template<typename T>
VarType type();

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
	int64_t ebp_;
	int64_t esp_;
	int64_t eip_;

	vector<int16_t> functions_;

	double deax, dvar1, dvar2, dvar3;
	int64_t ieax, ivar3;
	char *seax, *svar1, *svar2, *svar3;

	int8_t* stack_;

	template<typename T>
	T* popValue()
	{
		int64_t tmp = esp_;
		esp_ += sizeof(T);
		DEBUG_DO(cout << esp_ << ".." << esp_ + sizeof(T) << " pop value:" << *((T*)(stack_ + esp_)) << std::endl);
		return reinterpret_cast<T*>(stack_ + tmp);
	}

	template <typename T>
	void pushValue(T val)
	{
		esp_ -= sizeof(T);
		memcpy(stack_ + esp_, &val, sizeof(T));
		DEBUG_DO(cout << esp_ - sizeof(T) << ".." << esp_ << " push value:" << val <<std::endl);
	}
};

