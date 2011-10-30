#ifndef _BYTECODE_FUNCTION_H_
#define _BYTECODE_FUNCTION_H_

#include <set>

#include "mathvm.h"
#include "ast.h"

struct Bytecode: mathvm::Bytecode {
    uint8_t* bytecode() { return _data.data(); }
};

class BytecodeFunction: public mathvm::TranslatedFunction {
    Bytecode _bytecode;
    uint32_t _freeVarsNumber;
public:
    BytecodeFunction(mathvm::AstFunction* f): mathvm::TranslatedFunction(f) {}
    uint32_t freeVarsNumber() const { return _freeVarsNumber; }
    void setFreeVarsNumber(uint32_t f) { _freeVarsNumber = f; }
    Bytecode* bytecode() { return &_bytecode; }
    void disassemble(std::ostream& out) const { _bytecode.dump(out); }
};

#endif
