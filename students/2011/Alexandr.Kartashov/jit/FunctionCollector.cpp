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
  FunctionNode* node = af->node();

  _pool->addInfo(af->node());

  if (!node->body()->nodeAt(0)->isNativeCallNode()) {
    // This is a usual functon

    NativeFunction* nf = _runtime->createFunction(af);
  
    info(node)->funRef = nf;
    _functions.push_back(af);
    node->visit(this);

    info(node)->procAddress = NULL;
  } else {
    // This is an external function

    NativeCallNode* nc = node->body()->nodeAt(0)->asNativeCallNode();
    info(node)->procAddress = _runtime->addNativeFunction(nc->nativeName());

    if (!info(node)->procAddress) {
      ABORT("Failed to lookup %s!", nc->nativeName().c_str());
    }
  }  
}

