#include "FunctionCollector.h"

// ================================================================================

#define VISITOR FunctionCollector

using namespace mathvm;

FunctionCollector::FunctionCollector(AstFunction* root, Runtime* runtime, CompilerPool* pool) {
  collectFunctions(root, runtime, pool);
}

void FunctionCollector::collectFunctions(AstFunction* root, Runtime* runtime, CompilerPool* pool) {
  _runtime = runtime;
  _pool = pool;
  visit(root);
}

const FunctionCollector::Functions& FunctionCollector::functions() const {
  return _functions;
}

VISITQD(ForNode) {
  node->visitChildren(this);
}

VISITQD(WhileNode) {
  node->visitChildren(this);
}
    
VISITQD(IfNode) {
  node->visitChildren(this);
}
          
VISITQD(BlockNode) {
  Scope::FunctionIterator fi(node->scope());
  
  while (fi.hasNext()) {
    visit(fi.next());        
  }
}
    
VISITQD(FunctionNode) {
  node->visitChildren(this);
}

void FunctionCollector::visit(AstFunction* af) {
  Scope::VarIterator argsIt(af->scope());
  
  NativeFunction* nf = _runtime->createFunction(af);
  _pool->addInfo(af->node());
  info(af->node())->funRef = nf;
  
  _functions.push_back(af);
  
  af->node()->visit(this);
}

