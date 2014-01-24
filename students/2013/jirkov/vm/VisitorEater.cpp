#include <iostream>
#include <string>
#include <sstream>

#include "Translator.h"
#include "constants_pool.h"
#include "function.h"
#include "hash_table.h"
#include "cinterpreter.h"
#include "constants_pool.h"

namespace mathvm {
  
  static byte_t* get_bytecode( Bytecode& bc ) {
    const size_t size = bc.length();
    byte_t* bytes = new byte_t[size];
    for( size_t i = 0; i < size; i++ )
      bytes[i] = bc.get( i );
    
    return bytes;
  }
  
  static std::string flatten_signature( const Signature& sig ) {
    std::stringstream str; 
    for( Signature::const_iterator it = sig.begin(); it != sig.end(); ++it )
      str << (*it).second;
    return str.str();
  }
  
  void eat_visitor( MVMTranslator& translator ) 
  {
       
    constants_pool_init( translator.constant_pool.whole_size(), translator.constant_pool.count() );
    
    interpreter_init(); 
    
    //constants first  
    for( size_t i = 0; i < translator.constant_pool.count(); i++ )
      constants_pool_add( translator.constant_pool.get_strings().at(i).c_str() );
    
    for( std::vector<FunctionRecord*>::iterator it = translator.get_functions().begin();
	it != translator.get_functions().end(); it++)
	{ 
	  FunctionRecord* f = *it;
// 	  std::cout << f->name <<" , id = " << f->id << " is being eaten " << std::endl;
	  
	  //SO MUCH PAIN
	  byte_t* bytes = get_bytecode( f-> code);
	  
	  std::string sigstr = flatten_signature( f-> signature);
	  const char* sig = sigstr.c_str();
	  
	  table_put( f-> id, function_create( f-> id, sig, f-> code.length(), bytes) );
	  ctx_create_and_bind( f-> id, 1,0, f->locals_count *sizeof( cell_t ) , NULL );
	  //fixme: a proper stack size determined via abstract interpretation.
	  
	  delete[] bytes;	  
	} 
  } 
  
}
