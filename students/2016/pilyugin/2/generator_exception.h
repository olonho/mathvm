#ifndef GENERATOR_EXCEPTION_H
#define GENERATOR_EXCEPTION_H

#include <exception>
#include <cstdint>
#include <string>

namespace mathvm {

class BytecodeGeneratorException : public std::exception {
public:
    BytecodeGeneratorException(const std::string& message) :
        message_(message.c_str()) {}

    BytecodeGeneratorException(const char* message) :
        message_(message) {}

    char const* what() const throw() {
        return message_;
    }

private:
    char const* const message_;
};
}

#endif //GENERATOR_EXCEPTION_H