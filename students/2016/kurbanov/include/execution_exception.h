#pragma once

#include <string>
#include <exception>

class ExecutionException : public std::exception {
public:
    ExecutionException(std::string message)
            : mMessage(message) {}

    const char *what() const noexcept { return mMessage.c_str(); }

private:
    const std::string mMessage;
};

