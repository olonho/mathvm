#pragma once  
#include "helpers.h" 

namespace mathvm {
  
   struct  FunctionRecord { 
    FunctionRecord( std::string const& name,  Signature const& signature, const bool is_native ) 
    : name( name ),
    id( hash( name ) ),
    signature( signature ), 
    is_native( is_native ),
    locals_count( 0 ),
    stack_max( 0 )
    { }
    
    const std::string name;
    const uint16_t id;
    const Signature& signature;
    const bool is_native;
    size_t locals_count;
    size_t stack_max; 
    Bytecode code;
    
    const VarType return_type() const { return signature[0].first; }
    bool is_void() const { return return_type() == VT_VOID; }
    void update_stack_max( size_t value )  { stack_max = std::max( value, stack_max ); }
    void embrace( std::vector<Instruction> insns ) 
    { 
      for( std::vector<Instruction>::iterator it = insns.begin(); it != insns.end(); ++it )
	code.addInsn(*it);
    }
  };      
 
}