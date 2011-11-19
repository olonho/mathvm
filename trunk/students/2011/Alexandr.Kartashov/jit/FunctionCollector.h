#pragma once

#include "common.h"
#include "compiler.h"
#include "Runtime.h"

#include <map>

// ================================================================================

namespace mathvm {
  class FunctionCollector : private AstVisitor {
  public:
    typedef std::deque<AstFunction*> Functions;

    FunctionCollector(AstFunction* root, Runtime* runtime, CompilerPool* pool) {
      collectFunctions(root, runtime, pool);
    }

    void collectFunctions(AstFunction* root, Runtime* runtime, CompilerPool* pool) {
      _runtime = runtime;
      _pool = pool;
      visit(root);
    }

    const Functions& functions() const {
      return _functions;
    }

  private:
    VISIT(ForNode) {
      node->visitChildren(this);
    }

    VISIT(WhileNode) {
      node->visitChildren(this);
    }
    
    VISIT(IfNode) {
      node->visitChildren(this);
    }
          
    VISIT(BlockNode) {
      Scope::FunctionIterator fi(node->scope());

      while (fi.hasNext()) {
        visit(fi.next());        
      }
    }
    
    VISIT(FunctionNode) {
      node->visitChildren(this);
    }

    void visit(AstFunction* af) {
      Scope::VarIterator argsIt(af->scope());
        
      NativeFunction* nf = _runtime->createFunction(af);
      _pool->addInfo(af->node());
      info(af->node())->funRef = nf;

      _functions.push_back(af);
      
      af->node()->visit(this);
    }

  private:
    Runtime* _runtime;
    CompilerPool* _pool;
    Functions _functions;
  };
}
