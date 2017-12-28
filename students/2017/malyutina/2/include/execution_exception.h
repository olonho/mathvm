#ifndef EXECUTION_EXCEPTION_H__
#define EXECUTION_EXCEPTION_H__

#include <string>
#include <exception>

class exec_exception : public std::exception {
public:
    exec_exception(std::string message) : _message(message) {}

    const char *what() const noexcept { return _message.c_str(); }

private:
    const std::string _message;
};


#endif
