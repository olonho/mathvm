#ifndef MATHVM_TRANSLATOR_H
#define MATHVM_TRANSLATOR_H

#include "mathvm.h"

namespace mathvm {
    struct BytecodeTranslator : Translator {
        Status* translate(const std::string &program, Code **code) override;
    };
}
#endif //MATHVM_TRANSLATOR_H
