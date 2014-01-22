#pragma once

#include "../../../../include/mathvm.h"
#include "../../../../include/ast.h"

namespace mathvm {
  
 
  struct Annotation {
    Annotation( VarType type = VT_INVALID  ) : type( type ) {}
    VarType type;
  } ;
   
  
  Annotation* get_annotation( CustomDataHolder* holder );
  void set_annotation( CustomDataHolder* holder, Annotation* ann );
  
  
  void set_type( AstNode* node, VarType type );
  VarType get_type( AstNode* node );
  
}