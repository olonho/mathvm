#ifndef INTERPRETER_EXCEPTION_H
#define INTERPRETER_EXCEPTION_H

#include <exception>
#include <string>

namespace mathvm {

class InterpreterException : public std::exception {
public:
    InterpreterException(const std::string& message) :
            message_(message.c_str()) {}

    InterpreterException(const char* message) :
            message_(message) {}

    char const* what() const throw() {
        return message_;
    }

private:
    char const* const message_;
};
}

#endif //INTERPRETER_EXCEPTION_H
