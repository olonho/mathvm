#ifndef _MATHVM_ERROR_H
#define _MATHVM_ERROR_H

#include <stdexcept>

class TranslatorError: public std::runtime_error {
    uint32_t pos;
public:
    TranslatorError(const std::string &__arg, uint32_t position = 0) : runtime_error(__arg), pos(position)
    {}

    uint32_t getPosition() {
        return pos;
    }
};

class InterpreterError: public std::runtime_error {
public:
    InterpreterError(const std::string &__arg) : runtime_error(__arg) {}
};

#endif
