#pragma once

#include <stack>
using std::stack;

#include "MyMemoryManager.h"

class MyInstruction;
class MyBytecode;

class MyInterpreter
{
public:
	struct State
	{
		int programPointer_;
		stack<int>   returnPointers_;
		stack<void*> vals_;
		bool isExit_;
		State():isExit_(true), programPointer_(0){}
	};

	MyInterpreter(MyBytecode* bytecode);
	~MyInterpreter(void);

	void run();
private:
	MyBytecode* bytecode_;
	vector<string> strings_;

	State           state;
	MyMemoryManager memoryManager;
};

