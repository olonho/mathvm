
#include "AbstractInterpreter.h"


namespace mathvm {
  void AbstractInterpreter::embrace_vars(Scope* scope)
  { 
    for( Scope::VarIterator var( scope ); var.hasNext();  )
      _contexts-> add_var( var.next() );
    /*
    FunctionRecord* top = _contexts-> topmost_function_record();
    if (top != NULL) top-> locals_count += scope->variablesCount();*/
  } 
  void AbstractInterpreter::embrace_args(FunctionNode* node)
  {     
    for( size_t i = 0; i < node->parametersNumber(); ++i )
      _contexts-> add_var( node->parameterName( i ), node->parameterType( i ) );
  }
  void AbstractInterpreter::make_context()
  {
    _contexts = new StaticContext( _contexts ); 
  }
  
  void AbstractInterpreter::make_fun_context( FunctionNode* node, FunctionRecord* record )
  {
    _contexts = new FunctionContext( _contexts, record, node );
  }
  
  void AbstractInterpreter::destroy_context()
  { 
    
    StaticContext* old = _contexts; _contexts = _contexts-> previous; delete old; 
  }
  
  uint16_t AbstractInterpreter::current_context_id() 
  {
    return _contexts-> topmost_function_context()-> record-> id; 
  }
  
}