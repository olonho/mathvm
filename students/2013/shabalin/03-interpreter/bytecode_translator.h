#pragma once

#include "mathvm.h"
#include "ast.h"

//#define DEBUG
#ifdef DEBUG
#define loggerstr std::cerr
#else
struct null_stream : std::ostream {
   null_stream(): std::ostream(new std::filebuf()) { }
};

static null_stream loggerstr;

template<class T>
null_stream& operator<<(null_stream& ost, T const&) { return ost; }
#endif

namespace mathvm {

class BytecodeImpl : public Code {
   Bytecode* _bytecode;
public:
   BytecodeImpl() { _bytecode = new Bytecode(); }
   ~BytecodeImpl() { delete _bytecode; }
   virtual Status* execute(vector<Var*>& vars);
   virtual void disassemble(ostream& out = cout, FunctionFilter* filter = 0);
   Bytecode* bytecode() { return _bytecode; }
};
}
