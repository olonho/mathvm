#ifndef _TRANSLATOR_H
#define _TRANSLATOR_H

#include "mathvm.h"

#include <cstdio>
#include <string>

namespace mathvm {
  
class MvmCode : public Code {
 public:
    MvmCode() {
    }
    ~MvmCode() {}
    Status* execute(vector<Var*> vars) {
      // TBD
      return new Status();
    }
  };

struct ExtendedBytecode: Bytecode, MvmCode {};

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
