#include "util.h"
#include "sstream"
namespace mathvm {
    const char *escapechar(char c) {
        switch (c) {
            case '\n':
                return "\\n";
            case '\r':
                return "\\r";
            case '\t':
                return "\\t";
            case '\'':
                return "\\'";
            case '\\':
                return "\\\\";
            default:
                return NULL;
        }
    }

    std::string escape(const std::string &s) {
        std::string result;
        for (std::string::const_iterator c = s.begin(); c != s.end(); c++)
            if (escapechar(*c) == NULL) result += *c; else result += escapechar(*c);
        return result;
    }

    std::string toString(uint64_t i) {
        std::ostringstream result;
        result << i;
        return result.str();
    }

}