#pragma once

#include <mathvm.h>
using namespace mathvm;

#include <cstring>

#include "VarsUtil.h"

template<typename T>
VarType type();

template<> inline VarType type<int64_t>(){ return VT_INT; }
template<> inline VarType type<int64_t*>(){ return VT_STRING; }
template<> inline VarType type<double>(){ return VT_DOUBLE; }
template<> inline VarType type<double*>(){ return VT_STRING; }
template<> inline VarType type<string*>(){ return VT_STRING; }

class Interpreter
{
public:
	Interpreter();
	~Interpreter();

	void execute(const vector<Bytecode>& bytecodes, const vector<string>& literals);

private:
	void call(int id);

	vector<Bytecode> bytecodes_;
	vector<string>   literals_;

	int64_t ebp_;
	int64_t esp_;
	int64_t eip_;

	//int lastType_;
	//vector<VarType> types_;
	vector<int16_t> functions_;

	double deax, dvar1, dvar2, dvar3;
	int64_t ieax, ivar3;
	string *seax, *svar1, *svar2, *svar3;

	string emptyString_;

	int8_t* stack_;
	//vector<int8_t> stack_;

	template<typename T>
	T* popValue()
	{
		//lastType_--;
		esp_ -= sizeof(T);
		DEBUG_DO(cout << esp_ << ".." << esp_ + sizeof(T) << " pop value:" << *((T*)(stack_ + esp_)) << std::endl);
		return reinterpret_cast<T*>(stack_ + esp_);
	}

	template <typename T>
	void pushValue(T val)
	{
		memcpy(stack_ + esp_, &val, sizeof(T));
		//*((T*)(&stack_[0] + esp_)) = val;
		//types_[++lastType_] = type<T>();
		esp_ += sizeof(T);
		DEBUG_DO(cout << esp_ - sizeof(T) << ".." << esp_ << " push value:" << val <<std::endl);
	}
};

