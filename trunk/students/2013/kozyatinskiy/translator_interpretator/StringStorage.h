#pragma once

#include <vector>
using std::vector;
#include <string>
using std::string;

class StringStorage
{
public:
	int add(const string& str)
	{
		strings_.push_back(str);
		return strings_.size() - 1;
	}

	string get(int id)
	{
		return strings_[id];
	}

	const vector<string>& strings() const{ return strings_; }

private:
	vector<string> strings_;
};

