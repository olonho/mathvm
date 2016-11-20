#include <iostream>
#include "my_interpreter.h"

using namespace mathvm;

Status* InterpreterCodeImpl::execute(std::vector<Var*>& vars) {
  std::cerr << "TODO: Implement interpreter!." << std::endl;
  return Status::Error("Not implemented. It is next homework");
}
