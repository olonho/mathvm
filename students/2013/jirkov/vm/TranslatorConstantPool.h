#pragma once
#include <string>
#include <vector>
#include <algorithm>

namespace mathvm {
  class TranslatorConstantPool {
    std::vector<std::string> pool;
    size_t total_size;
    
  public: 
    typedef std::vector<std::string>::iterator iterator;
    TranslatorConstantPool() : total_size(0) { pool.push_back(""); total_size += 2; }
    uint16_t vivify( std::string const& str )
    {
      for( uint16_t i = 0; i < pool.size(); i++ )
	if ( pool.at(i) == str ) 
	  return i;
	
      pool.push_back( str );
      total_size += str.size()+1;
      return pool.size() - 1;
    
    }
    size_t count() { return pool.size(); }
    size_t whole_size() { return total_size; }
    std::vector<std::string>& get_strings() { return pool; }
  };
  
  
}