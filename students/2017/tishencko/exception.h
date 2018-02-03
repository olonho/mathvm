#ifndef EXCEPTION
#define EXCEPTION

#include <exception>
#include <string>

class Exception : public std::exception {
    std::string msg;
public:
    Exception(std::string const& message) : msg(message) {}

    const char* what() const throw() {
        return msg.c_str();
    }
    ~Exception() throw () {}
};

class TranslatorException: public Exception {
public:
    TranslatorException(const std::string& msg, int position) : Exception("Translator exception: " + msg + " in " + std::to_string(position)) {}
};

class InterpreterException: public Exception {
public:
    InterpreterException(const std::string& msg) : Exception("Interpreter exception: " + msg) {}
};

#endif // EXCEPTION

