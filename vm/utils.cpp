#include "mathvm.h"

#include <string>

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

using namespace std;

namespace mathvm {

char* loadFile(const char* file) {
    int fd = open(file, O_RDONLY);
    if (fd < 0) {
        return 0;
    }
    struct stat statBuf;
    if (fstat(fd, &statBuf) != 0) {
        return 0;
    }

    off_t size = statBuf.st_size;
    uint8_t* result = new uint8_t[size + 1];

    int rv = read(fd, result, size);
    assert(rv == size);
    result[size] = '\0';

    return (char*)result;
}

void positionToLineOffset(const string& text,
                          uint32_t position, uint32_t& line, uint32_t& offset) {
    uint32_t pos = 0, max_pos = text.length();
    uint32_t current_line = 1, line_start = 0;
    while (pos < max_pos) {
        while (pos < max_pos && text[pos] != '\n') {
            pos++;
        }
        if (pos >= position) {
            line = current_line;
            offset = position - line_start;
            return;
        }
        assert(text[pos] == '\n' || pos == max_pos);
        pos++; // Skip newline.
        line_start = pos;
        current_line++;
    }
}

const char* typeToName(VarType type) {
    switch (type) {
        case VT_INT: return "int";
        case VT_DOUBLE: return "double";
        case VT_STRING: return "string";
        case VT_VOID: return "void";
        default: return "invalid";
    }
}


VarType nameToType(const string& typeName) {
    if (typeName == "int") {
        return VT_INT;
    }
    if (typeName == "double") {
       return VT_DOUBLE;
    }
    if (typeName == "string") {
        return VT_STRING;
    }
    if (typeName == "void") {
        return VT_VOID;
    }
    return VT_INVALID;
}

} // namespace mathvm
