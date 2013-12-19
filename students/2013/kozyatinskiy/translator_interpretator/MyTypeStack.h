#pragma once

#include <vector>
#include "vm/mathvm.h"
using namespace mathvm;

class MyTypeStack
{
public:
	void push(VarType type)
	{
		types_.push_back(type);
	}
	VarType pop()
	{
		if (types_.empty())
			throw std::logic_error("empty stack");
		VarType tmp = types_.back();
		types_.pop_back();
		return tmp;
	}
	VarType top()
	{
		if (types_.empty())
			throw std::logic_error("empty stack");
		return types_.back();
	}
	VarType makeTwoSameNumeric()
	{
		if (types_.size() < 2) throw std::logic_error("empty stack");
		VarType secondType = types_[types_.size() - 2];
		if (secondType == VT_STRING) throw std::logic_error("can't convert string to numeric");
		return secondType;
	}
	VarType second()
	{
		if (types_.size() < 2) throw std::logic_error("empty stack");
		return types_[types_.size() - 2];
	}

private:
	vector<VarType> types_;
};

