#ifndef INTERPRETERCODEIMPL_H_
#define INTERPRETERCODEIMPL_H_

#include "mathvm.h"
#include "ast.h"
#include "parser.h"

namespace mathvm {

class InterpreterCodeImpl : public Code {
  public:
    virtual Status* execute(std::vector<mathvm::Var*>&);
};

}   // namespace mathvm

#endif /* INTERPRETERCODEIMPL_H_ */
