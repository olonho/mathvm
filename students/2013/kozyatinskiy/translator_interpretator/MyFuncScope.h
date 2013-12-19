#pragma once
#pragma once

#include <vector>
using std::vector;
using std::pair;
#include <string>
using std::string;
#include <algorithm>

#include "vm\mathvm.h"
using namespace mathvm;

class MyFuncScope
{
public:
	int saveFuncID(const Signature& signature, const string& name)
	{
		int id = getID(/*signature,*/ name);
		// add if vars dont exist
		if (id == funcs_.size())
			funcs_.push_back(std::make_pair(name, signature));
		// add var to current memory
		if (isInit_.empty())
			throw std::runtime_error("bad inc / dec mem use");
		isInit_.back().push_back(id);
		return id;
	}

	pair<VarType, int> getFuncID(/*const Signature& signature,*/ const string& name)
	{
		int id = getID(name);
		// add if vars dont exist
		if (id == funcs_.size())
			throw std::runtime_error("function not found");
		return std::make_pair(funcs_[id].second[0].first, id);
	}

	void incMem()
	{
		isInit_.push_back(vector<int>());
	}
	void decMem()
	{
		isInit_.pop_back();
	}

private:
	size_t getID(/*const Signature& signature, */const string& name)
	{
		size_t id = 0;
		// check if var already has id
		for (; id < funcs_.size(); ++id)
			if (/*isSignEqual(signature, vars_[id].first) && */funcs_[id].first.compare(name) == 0)
				break;
		return id;
	}

	vector<pair<string, Signature> > funcs_;
	vector<vector<int> > isInit_;

	bool isSignEqual(const Signature& s1, const Signature& s2)
	{
		if (s1.size() != s2.size()) return false;
		for (size_t i = 0; i < s1.size(); ++i)
			if (s1[i].first != s2[i].first || s1[i].second.compare(s2[i].second) != 0)
				return false;
		return true;
	}
};


