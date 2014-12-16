#include "errors.hpp"

#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <stdint.h>

namespace mathvm {

static void errorMessage(const char* program, char* buf, const char* message, uint32_t position) {
  uint32_t line = 0, offset = 0;
  positionToLineOffset(program, position, line, offset);
  sprintf(buf, 
          "Cannot translate program at line: %u, offset: %u, error: %s", 
          line, offset, message);
}

std::string errorMessage(const char* program, const char* message, uint32_t position) {
  char buf[constant::MAX_ERROR_MSG_LEN];
  errorMessage(program, buf, message, position);

  return buf;
}

std::string errorMessage(const char* program, const Status* status) {
  assert(status->isError());
  return errorMessage(program, status->getError().c_str(), status->getPosition());
}

TranslationException::TranslationException(const AstNode* at, const char* format, ...) {
    assert(at != 0);
    position_ = at->position();

    va_list args;
    va_start(args, format);
    vsnprintf(message_, constant::MAX_ERROR_MSG_LEN, format, args);
    va_end(args);
}

InternalException::InternalException(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(message_, constant::MAX_ERROR_MSG_LEN, format, args);
  va_end(args);
}

InterpreterException::InterpreterException(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(message_, constant::MAX_ERROR_MSG_LEN, format, args);
  va_end(args);
}

} // namespace mathvm