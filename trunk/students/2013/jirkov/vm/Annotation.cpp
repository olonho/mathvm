#include <stdexcept>
#include "Annotation.h"

namespace mathvm {
  
  void set_type( AstNode* node, VarType type ) { set_annotation( node, new Annotation( type ) ); }
  VarType get_type( AstNode* node ) 
  { 
    if ( node-> info() == NULL ) 
    {
      std::cerr <<  "No type for node at " << node->position() ; 
      return VT_VOID;
    }
    else  
      return get_annotation( node )-> type ; 
    
  }
  
  Annotation* get_annotation( CustomDataHolder* holder ) { return static_cast<Annotation*>( holder->info() ); }
  void set_annotation( CustomDataHolder* holder, Annotation* ann ) { holder->setInfo( ann ); }
void set_stack_size( AstNode* node, size_t value ) { get_annotation( node )->stack_size = value; }
  size_t get_stack_size( AstNode* node ) { return (get_annotation( node ) == NULL )?-1 : (get_annotation( node ))->stack_size; }
    
}
