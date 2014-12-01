#ifndef MY_UTILS_H
#define MY_UTILS_H

#include <string>
#include <exception>
#include <iostream>

#include "stdint.h"
#include "mathvm.h"

using std::string;
using std::exception;
using std::cout;

using namespace mathvm;

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

string instructionToString(Instruction insn);

void DEBUG_MSG(string const& msg);
void DEBUG_MSG(Bytecode* bc);
void DEBUG_MSG(vector<VarType> const& typesStack);

#endif // MY_UTILS_H
