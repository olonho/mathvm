#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <cstring>
#include <exception>
#include "mathvm.h"

using std::string;

namespace mathvm {

class MessageException : public std::exception
{
    string _message;

public:
    MessageException() {
    }

    MessageException(string const& message)
        : _message(message) {
    }

    virtual ~MessageException() throw() {
    }

    virtual const char* what() const throw() {
        return _message.c_str();
    }
};

class TranslationException: public MessageException
{
    uint32_t _position;

public:
    TranslationException() {
    }

    TranslationException(string const& message, uint32_t position = 0)
        : MessageException(message), _position(position) {
    }

    virtual ~TranslationException() throw() {
    }

    uint32_t position() const {
        return _position;
    }
};

class InterpretationException : public MessageException
{
    string _functionName;
    uint32_t _bytecodePosition;

public:
    InterpretationException(string const& message, string const& functionName = "", uint32_t bytecodePosition = 0)
        : MessageException(message), _functionName(functionName), _bytecodePosition(bytecodePosition) {
    }

    virtual ~InterpretationException() throw() {
    }

    const string& functionName() const {
        return _functionName;
    }

    uint32_t bytecodePosition() const {
        return _bytecodePosition;
    }
};


}


#endif // EXCEPTIONS_H
