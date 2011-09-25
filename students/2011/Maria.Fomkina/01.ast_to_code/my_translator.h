#ifndef _TRANSLATOR_H
#define _TRANSLATOR_H

#include "mathvm.h"

#include <cstdio>
#include <string>

namespace mathvm {
  
class MvmCode : public Code {
 public:
    MvmCode() {
      code_string = "";
    }
    ~MvmCode() {}
    Status* execute(vector<Var*> vars) {
      //printf("%s\n", code_string);
      return new Status();
    }
 private:
    std::string code_string;
  };

class CodeWriterTranslator : public Translator {
 public:
    CodeWriterTranslator() {}
    ~CodeWriterTranslator() {}
    Status* translate(const string& program, Code* *code);
  };
  
}

#endif /* TRANSLATOR_H_ */
