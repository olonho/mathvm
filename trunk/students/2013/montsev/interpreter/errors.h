#ifndef ERRORS_H
#define ERRORS_H

#include <string>
#include <sstream>
#include <stdexcept>

#include "mathvm.h"

// Exception class for type errors etc...
class error : public std::exception {
    std::string _msg;

public:
    explicit error(const std::string& msg): _msg(msg) {}
    explicit error(const char* msg): _msg(msg) {}

    virtual ~error() throw () {}

    const char* what() const throw () {
        return _msg.c_str();
    }
};

error invalidBytecodeError(uint16_t fid, uint32_t ip);
error stackUnderFlowError(uint16_t fid, uint32_t ip);
error notImplementedError(uint16_t fid, uint32_t ip);

#endif // ERRORS_H

