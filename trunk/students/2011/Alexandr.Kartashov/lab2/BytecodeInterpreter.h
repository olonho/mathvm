#pragma once

#include <vector>

#include "mathvm.h"

// ================================================================================

namespace mathvm {
  typedef std::vector<std::string> StringPool;

  union RTVar {
    int64_t i;
    double  d;
    char*   s;
  };


  class BytecodeInterpreter : public Code {
  public:
    void setVarPoolSize(unsigned int);

    Status* execute(vector<Var*> vars);
    
    Bytecode *bytecode();
    StringPool *strings();
    
  private:
    std::vector<RTVar> var_pool;
    StringPool string_pool;
    Bytecode code;
  };
}
