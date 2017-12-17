#ifndef TRANSLATION_EXCEPTION_H__
#define TRANSLATION_EXCEPTION_H__

#include <exception>

class translate_exception : public std::exception {
public:
    translate_exception(Status *status) : _status(status) {}

    const char *what() const noexcept { return _status->getError().c_str(); }

    Status *errorStatus() { return _status; }

private:
    Status *_status;
};

#endif
