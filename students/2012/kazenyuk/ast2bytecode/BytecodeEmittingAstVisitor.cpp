#include "BytecodeEmittingAstVisitor.h"

namespace mathvm_ext {

const uint8_t BytecodeEmittingAstVisitor::InsnSize[] = {
#define ENUM_ELEM(b, d, l) l,
    FOR_BYTECODES(ENUM_ELEM)
#undef ENUM_ELEM
    0
};

}
