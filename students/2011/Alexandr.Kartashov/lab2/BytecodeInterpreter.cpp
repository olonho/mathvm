#include "mathvm.h"
#include "BytecodeInterpreter.h"

// ================================================================================

namespace mathvm {
  Bytecode *BytecodeInterpreter::bytecode() {
    return &code;
  }

  StringPool *BytecodeInterpreter::strings() {
    return &string_pool;
  }

  void BytecodeInterpreter::setVarPoolSize(unsigned int pool_sz) {
    var_pool.reserve(pool_sz);
  }

  Status* BytecodeInterpreter::execute(vector<Var*> vars) {
    return 0;
  }  
};
