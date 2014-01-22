#include <stdexcept>
#include "AbstractContext.h"


namespace mathvm {
  
  FunctionContext* StaticContext::topmost_function_context() 
  {
    if ( is_function ) return reinterpret_cast<FunctionContext*>(this);
    else {
      if( previous == NULL ) throw logic_error( "Can't find any function context" );
      return previous-> topmost_function_context();
    }
  }
  FunctionContext* StaticContext::previous_function_context()
  {
    FunctionContext* top = topmost_function_context();
    if ( top == NULL ) return NULL;
    if ( top->previous == NULL ) return NULL;
    FunctionContext* prev = top->previous->topmost_function_context();
    if ( prev == NULL ) // root we are.
      return top;
    return prev;
  }
  
  FunctionRecord* StaticContext::topmost_function_record() 
  { 
    return topmost_function_context()->record;
  }
  
  size_t StaticContext::variables_inside_function()
  {
    if( previous == NULL || is_function) return variables.size();
    else return  variables.size() + previous-> variables_inside_function();
  }
  
  const Variable* const StaticContext::add_var( std::string const& name,  const VarType type ) 
  {  
    FunctionContext* fun = topmost_function_context();
    if ( fun == NULL ) return NULL;
    
    Variable* v = 
    &((*(variables.insert(make_pair(
      name, 
      Variable( (fun->record== NULL)? -1 : fun->record-> id,  name,  variables_inside_function(), type))).first)).second);
    //This whole abomination looks shit, but it's better than NOT using 'auto' anyways. 
    FunctionRecord* rec = fun->record;
    if (rec != NULL)
      rec->locals_count = std::max( rec->locals_count, variables_inside_function() );
    return v;
    
  }
  
  void StaticContext::add_fun( string const & name, Signature const & signature, bool is_native)
  {     
    FunctionContext* const fun = (previous == NULL)? topmost_function_context() : previous->topmost_function_context();
    
    if ( !fun->functions.insert(
      make_pair(name, new FunctionRecord( name, signature, is_native )))
      .second ) 
      throw logic_error( "Function name clashes with: " + name);
    
  }
  
  void StaticContext::add_fun( FunctionRecord* ptr )
  {    
    StaticContext*  fun = topmost_function_context();
    if ( fun == NULL ) return; 
    if ( fun-> previous != NULL ) fun = fun->previous;
    
    if ( !fun->functions.insert(make_pair(ptr->name,  ptr )).second ) 
      throw logic_error( "Function name clashes with: " + ptr-> name );
    
  }
  
  FunctionRecord* StaticContext::find_function( const string& name )
  {
    std::map<std::string, FunctionRecord*>::iterator it = functions.find( name );
    
        if ( it != functions.end()) return  it->second;
        if ( previous != NULL ) return previous-> find_function( name );
    return NULL;
  }
  
  const Variable* StaticContext::find_var( const string& name ) const
  {
    std::map<std::string, Variable>::const_iterator it = variables.find( name );
    
    if ( it != variables.end() ) return &( it->second );
    if ( previous != NULL ) return previous -> find_var( name );
    return NULL;
  }
  void StaticContext::assert_fun_exists( std::string const& name, std::string const& error_msg )
  {
    if ( find_function( name ) == NULL )
      throw logic_error( error_msg );
  }
  
}
