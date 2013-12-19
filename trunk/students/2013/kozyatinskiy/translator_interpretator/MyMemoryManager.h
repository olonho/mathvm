#pragma once

#include <string>
using std::string;
#include <vector>
using std::vector;

class MyMemoryManager
{
public:
	MyMemoryManager(void);
	~MyMemoryManager(void);

	void addConstString(const string& val)
	{
		constStrings_.push_back(val);
	}

	string* getConstString(int id)
	{
		return &(constStrings_[id]);
	}

	void* get(int id_)
	{
		int size = static_cast<int>(vars_.size());
		for (int i = size - 1; i >= 0; --i)
			if (vars_[i].size() > id_ && vars_[i][id_])
				return vars_[i][id_];
		throw std::logic_error("bad var");
	}

	void store(int id_, void* val)
	{
		for (int i = vars_.size() - 1; i >= 0; --i)
			if (vars_[i].size() > id_ && vars_[i][id_] != 0)
			{
				vars_[i][id_] = val;
				return;
			}
		vector<void*>& cur = vars_.back();
		if (cur.size() < id_ + 1)
			cur.resize(id_ + 1);
		cur[id_] = val;
	}

	void incMem()
	{
		vars_.push_back(vector<void*>());
	}

	void decMem()
	{
		vars_.pop_back();
	}
private:
	vector<string> constStrings_;

	vector<vector<void*> > vars_;
};

