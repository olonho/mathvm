#ifndef MY_UTILS_H
#define MY_UTILS_H

#include <string>
#include <exception>
#include <iostream>

#include "stdint.h"

using std::string;
using std::exception;
using std::cout;

class ExceptionWithMsg : public exception {
    string _whatMsg;
public:
    ExceptionWithMsg(string const& cause) : _whatMsg(cause) {}

    virtual const char* what() const throw() { return _whatMsg.c_str(); }

    virtual ~ExceptionWithMsg() throw () {}
};

class TranslatorException : public ExceptionWithMsg {
    size_t _pos;
public:
    TranslatorException(string const& cause, size_t pos)
        : ExceptionWithMsg(cause)
        , _pos(pos)
    {}

    size_t source() const { return _pos; }
};

void DEBUG_MSG(string const& msg);

#endif // MY_UTILS_H
