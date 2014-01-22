#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stddef.h>
#include "common.h"

namespace mathvm {
  typedef union {
    double as_double;
    int64_t as_int;
    char* as_ptr;
  } cell_t;
  
  /* SP always points at TOS, not near it.*/
  typedef struct 
  {
    const byte_t* IP;
    cell_t* SP;
  } registers_t;
  
  
  extern cell_t*  g_calc_stack;
  extern size_t g_calc_stack_count;
  
  
  void calc_stack_init( const size_t count );
  void calc_stack_deinit( void );
  
  void interpreter_init( void );
  
  void interpreter_start( byte_t* start );
  
  void interpreter_deinit( void );
  
}
#endif
