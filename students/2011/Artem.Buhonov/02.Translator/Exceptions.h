#pragma once

#include <string>

class Exception 
{
public:
	Exception() {}
	Exception(const std::string &what) : _what(what) {}
	const std::string& what() { return _what; }
private:
	std::string _what;
};
