#pragma once

#include <string>
#include <stdint.h>

 
#include "../../../../include/mathvm.h"

namespace mathvm {
 
  struct Variable {
    
    Variable( const uint16_t fun_id, std::string const& name, const uint16_t idx, VarType const& type) : 
    fun_id( fun_id ), 
    idx( idx ), 
    type( type ),
    name( name ) { }

    const uint16_t fun_id;
    const uint16_t idx;
    const VarType type;
    const std::string name;

    };
  
}