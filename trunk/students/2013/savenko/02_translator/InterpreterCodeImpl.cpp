#include <mathvm.h>

namespace mathvm {

class InterpreterCodeImpl : public Code {
public:
  InterpreterCodeImpl() {
  }

  virtual ~InterpreterCodeImpl() {
  }

  virtual Status* execute(std::vector<Var*> & vars) {
    return 0;
  }
};

}
