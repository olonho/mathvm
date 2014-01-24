#pragma once


#include <string.h> 
#include <fcntl.h>
#include <sys/stat.h> 
#include <iostream>
#include <list>
#include "helpers.h"
#include <stdexcept>

#include "../../../../include/ast.h"
#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"
#include "../../../../vm/parser.h" 


#include "AbstractContext.h"
#include "FunctionRecord.h"
#include "TranslatorConstantPool.h"




namespace mathvm { 
  
  class AbstractInterpreter : public AstBaseVisitor {
    
  public:
    virtual void pre_node_actions( AstNode* node ) = 0;
    AbstractInterpreter() : _contexts( NULL ){}
    void visit_scope_functions( Scope* scope ) { 
      for ( Scope::FunctionIterator it( scope ) ; it.hasNext(); ) 
      {
	FunctionNode* f = it.next()->node();
	visitFunctionNode(f); 
      } 
    }
    
    uint16_t current_context_id();
    
    virtual void make_context();    
    virtual void make_fun_context( FunctionNode* node, FunctionRecord* record );
    virtual void destroy_context();
    
    std::vector<FunctionRecord*>& get_functions() { return _functions; }
    
    
    void embrace_vars( Scope* scope );
    
    void embrace_args(FunctionNode* node);
    
  protected:
    StaticContext* _contexts;
    
    std::vector< FunctionRecord* > _functions;
  };
  
}