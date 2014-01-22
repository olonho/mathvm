#pragma once

#include "Variable.h" 
#include "FunctionRecord.h"
 
namespace mathvm {
  
  struct FunctionContext;
  
  
  struct StaticContext {
    
//     StaticContext( StaticContext* old,  Scope* scope ): previous( old ),  is_function( old-> is_function )  {
//       for ( Scope::VarIterator it( scope ); it.hasNext(); ) 
//       {
// 	const AstVar* var = it.next(); 
// 	add_var( var->name(),  var->type());
//       }
//     }
//     
    StaticContext( StaticContext* prev ) : previous( prev ),  is_function( false ) {}
    
    
    std::map<std::string, FunctionRecord* > functions; 
    StaticContext* previous; 
    bool is_function;  
    
    std::map<std::string, Variable> variables; 
    
    
    void assert_fun_exists( std::string const& name, std::string const& error_msg );
    
    FunctionRecord* find_function( std::string const& name ); 
    
    const Variable* find_var( std::string const& name ) const;
    
    
    FunctionContext* topmost_function_context(); 
    FunctionRecord* topmost_function_record();
    FunctionContext* previous_function_context(); 
    
    const Variable *const add_var( std::string const& name,  const VarType type ) ;
    
    const Variable *const add_var( const AstVar* var )  { return add_var( var-> name(), var-> type() ); }
    
    
    
    void add_fun( FunctionNode const* node ) {
      add_fun ( node-> name(),  node-> signature(), false );
    }
    
    void add_fun( FunctionRecord* rec );
    
    void add_fun( string const & name, Signature const & signature, bool is_native); 
    
    size_t variables_inside_function();
    
    ~StaticContext() {}
    void destroyDescendants(){ if ( previous != NULL ) { previous-> destroyDescendants(); delete previous; } }
    
  protected :
    StaticContext(  StaticContext* const prev,  bool is_function) : previous(prev), is_function( is_function ) {}
    StaticContext(  bool is_function) : is_function( is_function ) {}
    
    
  };
  
  struct FunctionContext : public StaticContext {
    FunctionContext( StaticContext* prev, FunctionRecord* record, FunctionNode* node ) 
    : StaticContext( prev,  true ), 
    record( record ),  
    variables_count( 0 ), 
    node( node ){ }
    FunctionRecord  *const record;
    
    size_t variables_count;
    FunctionNode* const node;
  };
  
 
}