#ifndef TRANSLATOR_EXCEPTION_H
#define TRANSLATOR_EXCEPTION_H

#include <stdexcept>
#include <string>

class translator_exception : public std::runtime_error
{
public:
    translator_exception() = default;

    translator_exception(std::string message)
        : runtime_error(message)
    {}
};

#endif // TRANSLATOR_EXCEPTION_H
