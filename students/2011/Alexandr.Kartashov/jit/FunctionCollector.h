#include "common.h"
#include "compiler.h"
#include "Runtime.h"

// ================================================================================

namespace mathvm {
  class FunctionCollector : private AstVisitor {
    Runtime* _runtime;
    CompilerPool* _pool;

  public:
    FunctionCollector() { }

    void collectFunctions(AstNode* root, Runtime* runtime, CompilerPool* pool) {
      _runtime = runtime;
      _pool = pool;
      root->visit(this);
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
        AstFunction* af = fi.next();
        Scope::VarIterator argsIt(af->scope());
        
        NativeFunction* nf = _runtime->createFunction(af);
        _pool->addInfo(af->node());
        info(af->node())->funRef = nf;
      }
    }
    
    VISIT(FunctionNode) {
      node->visitChildren(this);
    }
  };
}
