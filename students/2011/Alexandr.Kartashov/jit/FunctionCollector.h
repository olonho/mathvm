#pragma once

#include "common.h"
#include "compiler.h"
#include "Runtime.h"

#include <map>

// ================================================================================

class Runtime;

namespace mathvm {
  class FunctionCollector : private AstVisitor {
  public:
    typedef std::deque<AstFunction*> Functions;

    FunctionCollector(AstFunction* root, Runtime* runtime, CompilerPool* pool);

    void collectFunctions(AstFunction* root, Runtime* runtime, CompilerPool* pool);

    const Functions& functions() const;

  private:
    VISIT(ForNode);
    VISIT(WhileNode);
    VISIT(IfNode);
    VISIT(BlockNode);
    VISIT(FunctionNode);

    void visit(AstFunction* af);

  private:
    Runtime* _runtime;
    CompilerPool* _pool;
    Functions _functions;
  };
}
