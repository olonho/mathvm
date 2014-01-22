#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "function.h"

#define HASH_MODULO 65537
#define HASH_TABLE_SIZE 65536
namespace mathvm {
  
  vm_id_t hash( const char* str );
  
  
  extern function_t* hash_table[HASH_TABLE_SIZE];
  
  
  function_t* const table_get( vm_id_t hash );
  
  void table_put( vm_id_t hash, function_t* const header );
  
  function_t* table_remove( const vm_id_t hash );
}

#endif
