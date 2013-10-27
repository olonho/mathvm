#pragma once

#include "mathvm.h"
#include "ast.h"

namespace mathvm {

class BytecodeImpl : public Code {
   Bytecode* _bytecode;
public:
   BytecodeImpl() { _bytecode = new Bytecode(); }
   ~BytecodeImpl() { delete _bytecode; }
   virtual Status* execute(vector<Var*>& vars) { return NULL; }
   virtual void disassemble(ostream& out = cout, FunctionFilter* filter = 0);
   Bytecode* bytecode() { return _bytecode; }
};
}
