#pragma once

#include <stdint.h>

struct StackValue{
	union
	{
		double _doubleValue;
		int64_t _intValue;
		const char* _strPtr;
	};
	StackValue(double value) : _doubleValue(value) {}
	StackValue(int64_t value) : _intValue(value) {}
	StackValue(const char* strPtr) : _strPtr(strPtr) {}
};

class DataStack
{
	uint64_t _stackSize;
	StackValue *_stack;
	StackValue *_top;
	bool _isEmpty;

public:
	DataStack(uint64_t size = 2048*100);
	~DataStack(void);
	StackValue pop();
	void push(StackValue value);
	bool isEmpty();
};

