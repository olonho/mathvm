#include "Runtime.h"

// ================================================================================

typedef void (*CompiledFunc)(void);

namespace mathvm {
  NativeFunction* Runtime::createFunction(AstFunction* fNode) {
    _functions.push_back(new NativeFunction(fNode));

    return _functions.back();
  }

  Status* Runtime::execute(std::vector<mathvm::Var*, std::allocator<Var*> >& args) { 
    NativeCode* c = _functions[0]->code();
    //c->put(0, 0);
    CompiledFunc f = (CompiledFunc)c->x86code();
    f();

    return NULL;
  }
}
