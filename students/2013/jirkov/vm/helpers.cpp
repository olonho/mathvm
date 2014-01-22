#include "helpers.h"
#include "hash_table.h"
#include <string>
#include "../../../../include/ast.h"
#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"
#include "../../../../vm/parser.h" 

namespace mathvm {
  const int16_t hash( std::string const & str ) 
  {
    int16_t acc = 0;
    for( std::string::const_iterator iter = str.begin(); iter != str.end(); ++iter)
    {
      acc *= 29;
      acc %= HASH_MODULO;
      acc += *iter;
      acc %= HASH_MODULO;
    }
    return acc;
  }
  
  const int16_t hash_signature( Signature const& sig ) {
    return hash( sig.at(0).second );
  }
}