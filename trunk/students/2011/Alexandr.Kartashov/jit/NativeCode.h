#include <vector>

#include "mathvm.h"

// ================================================================================

namespace mathvm {
  class NativeCode : public Code {
    Status* execute(std::vector<mathvm::Var*, std::allocator<Var*> >&);
  }
}
