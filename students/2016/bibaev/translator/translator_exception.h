#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>

namespace mathvm {
class TranslationException : public std::logic_error {
public:
  TranslationException(std::string const& what, uint32_t position)
      : logic_error(what), _position(position) {

  }

  inline uint32_t position() const {
    return _position;
  }

private:
  uint32_t const _position;
};
}
