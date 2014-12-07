#pragma once

#include <string>

namespace mathvm {

    const char *escapechar(char c);

    std::string escape(std::string const &s);
    std::string toString(uint64_t i);
}