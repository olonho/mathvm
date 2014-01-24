#pragma once

#include "../../../../include/mathvm.h"
#include "../../../../include/ast.h"

namespace mathvm {
  
 
  struct Annotation {
    Annotation( VarType type = VT_INVALID  ) : type( type ), stack_size(-1) {}
    VarType type;
    size_t stack_size;
  } ;
   
  
  Annotation* get_annotation( CustomDataHolder* holder );
  void set_annotation( CustomDataHolder* holder, Annotation* ann );
  
  
  void set_type( AstNode* node, VarType type );
  VarType get_type( AstNode* node );
  void set_stack_size( AstNode* node, size_t value );
  size_t get_stack_size( AstNode* node );
  
}