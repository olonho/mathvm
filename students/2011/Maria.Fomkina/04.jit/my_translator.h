#ifndef _TRANSLATOR_H
#define _TRANSLATOR_H

#include "mathvm.h"
#include "mvm_code.h"

#include <cstdio>
#include <string>

namespace mathvm {
  
class BytecodeTranslator : public Translator {
 public:
    BytecodeTranslator() {}
    ~BytecodeTranslator() {}
    Status* translate(const string& program, Code* *code);
    
 private:
    Bytecode* bytecode_;
  };
  
}

#endif /* TRANSLATOR_H_ */
