#ifndef BYTECODE_TRANSLATOR_H
#define BYTECODE_TRANSLATOR_H

#include "mathvm.h"

namespace mathvm {

class BytecodeTranslator : public Translator {
    Status* translateBytecode(const string& program, InterpreterCodeImpl* *code);

  public:
    virtual ~BytecodeTranslator() {}

    virtual Status* translate(const string& program, Code* *code);
};

}

#endif // BYTECODE_TRANSLATOR_H
