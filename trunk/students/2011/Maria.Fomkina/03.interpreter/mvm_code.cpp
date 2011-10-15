#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "mvm_code.h"
#include "bytecode_visitor.h"

namespace mathvm {

Status* MvmCode::execute(vector<Var*>& vars) {
  printf("Please, implement interpreter!\n");
  return new Status();
}

}
