#pragma once

#include <stdexcept>
#include <string>

#include "mathvm.h"

using namespace mathvm;

class TranslateError : public exception
{
public:
	TranslateError(const string& msg, uint32_t pos = Status::INVALID_POSITION)
	: msg(msg)
	, pos(pos)
	{}

	virtual ~TranslateError() throw() {}

	virtual const char* what() {
		return msg.c_str();
	}
	uint32_t position() {
		return pos;
	}

private:
	string msg;
	uint32_t pos;
};
