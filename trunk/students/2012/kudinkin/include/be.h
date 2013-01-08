//
//  Bytecode Executable (BE) for MVM
//

#ifndef _MATHVM_BYTECODE_EXECUTABLE_H_301212_
#define _MATHVM_BYTECODE_EXECUTABLE_H_301212_

#include "mathvm.h"


namespace mathvm
{
  
  class BytecodeExecutable : public Code {
    
    virtual Status* execute(vector<Var*>& vars) { return new Status(); };
    
  };
  
}

#endif // #ifndef _MATHVM_BYTECODE_EXECUTABLE_H_301212_

