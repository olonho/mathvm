#ifndef RESULT_BYTECODE_H__
#define RESULT_BYTECODE_H__

#include "mathvm.h"

#include <string>

using namespace mathvm;

class ResultBytecode: public Code {
  public:
    virtual Status* execute(vector<Var*>& vars) {
      return Status::Ok();
    }

    virtual void disassemble(ostream& out = cout, FunctionFilter* filter = 0) {
      const std::string topFunctionName = "<top>";
      auto topFunction = functionByName(topFunctionName);
      topFunction->disassemble(out);
    }
};

#endif
