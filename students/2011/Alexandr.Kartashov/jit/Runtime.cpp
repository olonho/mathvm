#include "Runtime.h"

// ================================================================================

extern "C" void icall(const void* p);

namespace mathvm {
  NativeFunction* Runtime::createFunction(AstFunction* fNode) {
    _functions.push_back(new NativeFunction(fNode));

    return _functions.back();
  }

  Status* Runtime::execute(std::vector<mathvm::Var*, std::allocator<Var*> >& args) { 
    icall(&_functions[0]->code()->data()[0]);

    return NULL;
  }
}
