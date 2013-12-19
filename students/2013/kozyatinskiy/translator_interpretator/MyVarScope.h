#pragma once

#include <vector>
using std::vector;
using std::pair;
#include <string>
using std::string;
#include <algorithm>

#include "vm\mathvm.h"
using namespace mathvm;

class MyVarScope
{
public:
	int getStoreID(VarType type, const string& name)
	{
		int id = getID(type, name);
		// add if vars dont exist
		if (id == vars_.size())
		{
			vars_.push_back(std::make_pair(type, name));
		}
		// add var to current memory
		if (isInit_.empty())
			throw std::runtime_error("bad inc / dec mem use");
		isInit_.back().push_back(id);
		return id;
	}

	int getLoadID(VarType type, const string& name)
	{
		int id = getID(type, name);
		// add if vars dont exist
		if (id == vars_.size())
			throw std::runtime_error("try load bad variable");
		for (int i = isInit_.size() - 1; i >= 0; --i)
			for (size_t j = 0; j < isInit_[i].size(); ++j)
				if (isInit_[i][j] == id)
					return id;
		throw std::runtime_error("try load bad variable");
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
	int getID(VarType type, const string& name)
	{
		size_t id = 0;
		// check if var already has id
		for (; id < vars_.size(); ++id)
			if (vars_[id].first == type && vars_[id].second.compare(name) == 0)
				break;
		return id;
	}

	vector<pair<VarType, string> > vars_;
	vector<vector<int> > isInit_;
};

