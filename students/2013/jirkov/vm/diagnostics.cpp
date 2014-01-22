#include "diagnostics.h"
#include "context.h"
#include "hash_table.h"
#include "instruction_set.h"

namespace mathvm { 
  //todo not used?
  //void function_print( const function_t* const fun, FILE* f )
  //{
  //	fprintf( f, "function: %s\n"
  //		"id=%x\tcode_size=%u\tstack=%d\n"
  //		"locals: %u bytes",
  //		fun-> signature, 
  //		fun-> static_ctx-> id,
  //		fun-> code_size,
  //		fun-> static_ctx-> stack_size,
  //		fun-> static_ctx-> locals_size);
  //}
  
  void bytecode_dump( const byte_t* bc, FILE* file )
  {
//     size_t size = 1;
    size_t offset = *bc;
    fprintf( file, "%s ", bytecode_mnemonic[ offset ] );
//     fprintf( file, "%s ", bytecode_mnemonic[*bc]);
  }
  
  size_t instruction_dump( const byte_t* const bytes, FILE* file )
  {
    #define PRINT_AND_GET_SIZE( b, s, l ) case BC_##b: fprintf( file, #b ); instr_size = l; break; 
    
    size_t instr_size;
    size_t i;
    //bytecode_dump( bytes, file );
    
    
    switch( *bytes )
    {
      FOR_BYTECODES( PRINT_AND_GET_SIZE )
      default: instr_size = 0; break;
    }
    
    for( i = 1;  i < instr_size; i++ )
      fprintf( file, "%2.2X", bytes[i] );
    fputs( "\n", file );
    
    return instr_size;
    
    #undef PRINT_AND_GET_SIZE
  }
  
  void function_dump( const function_t* const fun, FILE* file)
  {
    size_t i;
    
    fprintf( file,  "\nfunction %s\n id %ccccx \n",  fun-> signature, fun-> id );
    for( i = 0; i < fun->code_size; i += instruction_dump( fun-> code + i, file ) );
    
    ctx_static_dump( fun-> static_ctx, file );
  }
  
  void ctx_static_dump( const ctx_static_t* const ctx, FILE* file )
  {
    //     size_t i;
    //     fprintf( file, "Static context stack size: %lu bytes\n default locals values: ", ctx->stack_size );
    //     
    //     for( i = 0; i < ctx->locals_size; i++ )
    //       fprintf( file, "%hhx", ( ctx->is_zero_initialized )? 0 : *( (byte_t*)( ctx->locals_defaults ) + i ) );
    //     if ( ctx->locals_size == 0 ) fprintf( file, "none\n" );
    //     else fprintf( file, "\n" );
  }
  
  void table_dump( FILE* file )
  {
    size_t i;
    for( i = 0; i < HASH_TABLE_SIZE; i++ ) 
      if ( hash_table[i] != NULL )
      {
	fprintf( file, "\n hashtable entry No %lu: ", i );
	function_dump(hash_table[i], file);
      }
  }
  
  void registers_dump( const registers_t* const regs, FILE* f )
  {
    fprintf( f, "IP: %p SP: %lx %lf %p \n", 
	     regs-> IP, 
	     regs-> SP-> as_int, 
	     regs-> SP-> as_double, 
	     regs-> SP-> as_ptr
    );
  }
  /*
   * extern size_t g_calc_stack_count;*/
  
  void calc_stack_dump( FILE* file )
  {//quick and dirty
    //     cell_t* c;
//     size_t i = 0;
//     fprintf( file, "\n\nstack : \n" );
//     
//     //     for(c = g_registers.SP; c != g_calc_stack + g_calc_stack_count-1; c++ )
//     //       fprintf( file, "%lx %lf %p\n", c->as_int, c->as_double, c->as_ptr );
//     //     
//     for( i = 0; i < 20; i++ )
//     {
//       cell_t* c = &( g_registers.SP[i]);
//       fprintf( file, "%lx %lf %p\n", c->as_int, c->as_double, c->as_ptr );
//     }
  }
  
}