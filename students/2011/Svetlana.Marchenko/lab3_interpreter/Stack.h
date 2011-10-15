#pragma once

#include <assert.h>
#include <iostream>

#define STACK_SIZE 1024*1024;

class Stack {

private:
	unsigned char* _data;
	size_t _tos;
public:
	Stack() {
		_data = new unsigned char[STACK_SIZE];
		_tos = _data;
	}
	
	~Stack() {
		delete[] _data;
	}
	
	template <typename T>
	void push(T t) {
		size_t var_size = sizeof(T);
		assert((_tos + var_size < _data + STACK_SIZE) && cerr << "Stack overflow");
		*(T*)(_tos) = t;
		_tos += var_size;
	}
	
	template <typename T>
	T pop() {
		size_t var_size = sizeof(T);
		assert((_tos - var_size >= _data) && cerr << "Out of stack bounds");
		T t = *(T*)(_tos - var_size);
		_tos += var_size;
		return t;
	}
	
	template <typename T>
	T get() {
		size_t var_size = sizeof(T);
		assert((_tos - var_size >= _data) && cerr << "Out of stack bounds");
		T t = *(T*)(_tos - var_size);
		return t;
	}

};
