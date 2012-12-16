#ifndef INTERPRETERCODEIMPL_H_
#define INTERPRETERCODEIMPL_H_

#include "mathvm.h"
#include "ast.h"
#include "parser.h"

namespace mathvm {

class InterpreterCodeImpl : public Code {
  public:
  	InterpreterCodeImpl(bool trace = false)
  		: m_trace(trace)
  	{}
    virtual Status* execute(std::vector<Var*>&);
  private:
  	void decodeInsn(Bytecode* bytecode, size_t bci, Instruction& insn, size_t& length);
  	bool m_trace;
};

}   // namespace mathvm

#endif /* INTERPRETERCODEIMPL_H_ */
