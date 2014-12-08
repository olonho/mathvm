#ifndef MY_UTILS_H
#define MY_UTILS_H

#include <string>
#include <exception>

#include "stdint.h"
#include "ast.h"

using std::string;
using std::exception;

using namespace mathvm;

class ExceptionWithMsg : public exception {
    string _whatMsg;
public:
    ExceptionWithMsg(string const& cause) : _whatMsg(cause) {}

    virtual const char* what() const throw() { return _whatMsg.c_str(); }

    virtual ~ExceptionWithMsg() throw () {}
};

class ExceptionWithPos : public ExceptionWithMsg {
    size_t _pos;
public:
    ExceptionWithPos(string const& cause, size_t pos)
        : ExceptionWithMsg(cause)
        , _pos(pos)
    {}

    size_t source() const { return _pos; }
};

class ExecStatus {
    bool _is_error;
    string _cause;
    size_t _pos;

public:
    ExecStatus(): _is_error(false), _cause(), _pos(0) {}

    void setError(string const& cause, size_t pos) {
        _is_error = true;
        _cause = cause;
        _pos = pos;
    }
    void setOk() { _is_error = false; }

    bool isError() const { return _is_error; }
    string const& errCause() const { return _cause; }
    size_t errPos() const { return _pos; }
};

void DEBUG_MSG(string const& msg);
void DEBUG_MSG(Scope* scope);

#endif // MY_UTILS_H
