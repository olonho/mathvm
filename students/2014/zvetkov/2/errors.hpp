#ifndef ERRORS_HPP
#define ERRORS_HPP

#include "ast.h"

#include <exception>

namespace mathvm {

namespace constant {
  const int MAX_ERROR_MSG_LEN = 256;
}


std::string errorMessage(const char* program, const char* message, uint32_t position);
std::string errorMessage(const char* program, const Status* status);


class TranslationException : public std::exception {
  char message_[constant::MAX_ERROR_MSG_LEN];
  uint32_t position_;

public:
  TranslationException(const AstNode* at, const char* format, ...);

  virtual const char* what() const throw() {
    return message_;
  }

  uint32_t position() const {
    return position_;
  }
};

class InternalException : public std::exception {
  char message_[constant::MAX_ERROR_MSG_LEN];

public:
  InternalException(const char* format, ...);

  virtual const char* what() const throw() {
    return message_;
  }
};

class InterpreterException : public std::exception {
  char message_[constant::MAX_ERROR_MSG_LEN];

public:
  InterpreterException(const char* format, ...);

  virtual const char* what() const throw() {
    return message_;
  }
};

} // namespace mathvm

#endif