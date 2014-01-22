#pragma once

#include "../../../../include/mathvm.h"
#include "../../../../include/ast.h"

  
namespace mathvm {
  
#define MKSTR( name, str, n ) #name ,
  
  static const char* const TOKENS[] = { 
    FOR_TOKENS( MKSTR )
    "count"
  };
  static const char* const TYPES[] = {
    "INVALID",
    "VOID",
    "DOUBLE",
    "INT",
    "STRING"
  
};
#define token_name( tok ) TOKENS[ tok ]
#define type_string( type ) TYPES[ type ]
  
const int16_t hash( std::string const& str );
const int16_t hash_signature( Signature const& sig );

}