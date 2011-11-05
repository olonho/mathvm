#include "common.h"
#include "compiler.h"
#include "Runtime.h"

// ================================================================================

namespace mathvm {
  class FunctionCollector : private AstVisitor {
    Runtime* _runtime;
    CompilerPool* _pool;

  public:
    FunctionCollector(AstFunction* root, Runtime* runtime, CompilerPool* pool) {
      collectFunctions(root, runtime, pool);
    }

    void collectFunctions(AstFunction* root, Runtime* runtime, CompilerPool* pool) {
      _runtime = runtime;
      _pool = pool;
      visit(root);
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
      
      af->node()->visit(this);
    }
  };
}
