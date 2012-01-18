#include "DataStack.h"

#include <string.h>

DataStack::DataStack(uint64_t size) : _stackSize(size), _isEmpty(true)
{
	_stack = (StackValue *)(new char[_stackSize]);
	_top = _stack;
}


DataStack::~DataStack(void)
{
	delete[] (char *)_stack;
}

StackValue DataStack::pop() {
	StackValue *val = (StackValue *)(_top - sizeof(struct StackValue));
	_top -= sizeof(struct StackValue);
	if (_top == _stack) _isEmpty = true;
	return *val;
}

void DataStack::push(StackValue value) {
	if (_isEmpty) _isEmpty = false;
	memcpy(_top, &value, sizeof(struct StackValue));
	_top += sizeof(struct StackValue);
}

bool DataStack::isEmpty() { return _isEmpty; }