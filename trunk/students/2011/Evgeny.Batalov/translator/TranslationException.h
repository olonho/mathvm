#pragma once 
#include <exception>
#include <string>

class TranslationException : public std::exception {
    std::string error;
public:
    TranslationException(const std::string& err)
        :   error(err) 
        {}
    virtual const char* what() {
        return error.c_str();
    }

    virtual ~TranslationException() throw() {}
};

