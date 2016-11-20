#ifndef _TRANSLATOREXCEPTION_H
#define _TRANSLATOREXCEPTION_H

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

#endif
