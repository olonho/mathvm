#include "BytecodeInstructionPrimitives.h"

namespace mathvm_ext {

using namespace mathvm;

BytecodeInstructionPrimitives::BytecodeInstructionPrimitives() {
}

BytecodeInstructionPrimitives::~BytecodeInstructionPrimitives() {
}

const uint8_t BytecodeInstructionPrimitives::InsnSize[] = {
#define ENUM_ELEM(b, d, l) l,
    FOR_BYTECODES(ENUM_ELEM)
#undef ENUM_ELEM
    0
};

} /* namespace mathvm_ext */
