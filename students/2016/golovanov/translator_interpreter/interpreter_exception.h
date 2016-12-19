#ifndef INTERPRETER_EXCEPTION_H
#define INTERPRETER_EXCEPTION_H

#include <stdexcept>
#include <string>

class interpreter_exception : public std::runtime_error
{
public:
    interpreter_exception() = default;

    interpreter_exception(std::string message)
        : runtime_error(message)
    {}
};

#endif // INTERPRETER_EXCEPTION_H
