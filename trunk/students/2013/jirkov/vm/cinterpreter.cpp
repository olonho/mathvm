
#include <cstdio>
#include <malloc.h>
#include <setjmp.h>
#include <string.h>

#include "common.h"
#include "context.h"
#include "constants_pool.h"
#include "instruction_set.h"
#include "diagnostics.h"


namespace mathvm {
  
  cell_t*  g_calc_stack;
  size_t g_calc_stack_count = 0;
  
  
  #define GET_CELL( byte_ptr ) (*((cell_t*)(byte_ptr))) 
  
  #define GET_WORD( byte_ptr ) (*((uint16_t *)(byte_ptr)))
  
  jmp_buf stop_label;
  
  
  
  void calc_stack_init( const size_t count )
  {
    g_calc_stack = (cell_t*) calloc( count, sizeof( cell_t ) );
    g_calc_stack_count = count;
  }
  
  void calc_stack_deinit( void )
  {
    free( g_calc_stack );
  }
  
  void interpreter_init( void )
  {
    ctx_stack_init( 65536*16 ); 
    calc_stack_init( 65536*16 );
  }
  
  void interpreter_deinit( void )
  {
    ctx_stack_deinit();
    calc_stack_deinit();
    constants_pool_deinit();
  }
  
   
  
  void interpreter_start( byte_t* start )
  {
    volatile bool_t launched = false; 
    registers_t regs;
    
    regs.IP = start;
    regs.SP = g_calc_stack + g_calc_stack_count-1;
     
    setjmp( stop_label );
    
    if ( ! launched )
    {
      launched = true;
      for( ;; )
      { 
//  	instruction_dump( regs.IP-1, stderr );
	switch(*(regs.IP++))
	{ 
	  
	  
	  case BC_INVALID: break;
	  
	  
	  case BC_SLOAD:
	    regs.SP--;
	    regs.SP->as_ptr = (char*)( constant_pool_get( GET_WORD( regs.IP ) ) );
	    regs.IP += sizeof( uint16_t );
	    break;
	    
	  case BC_DLOAD:
	  case BC_ILOAD:
	    regs.SP--;
	    regs.SP[0] = GET_CELL( regs.IP );
	    regs.IP += sizeof( cell_t );
	    break;
	     
	    
	  case BC_SLOAD0:
	    regs.SP--;
	    regs.SP-> as_ptr = (char*) constant_pool_get( 0 );
	    break;
	  case BC_ILOAD0:
	  case BC_DLOAD0:
	    regs.SP--;
	    regs.SP-> as_int = 0;
	  break; 
	  case BC_DLOAD1:
	    regs.SP--;
	    regs.SP-> as_double = 1.0;
	  break;
	  case BC_ILOAD1:
	    regs.SP--;
	    regs.SP-> as_int = 1;
	  break;
	  case BC_DLOADM1:
	    regs.SP--;
	    regs.SP-> as_double = -1.0;
	  break;
	  
	  case BC_ILOADM1:
	    regs.SP--;
	    regs.SP-> as_int = -1;
	  break;
	  
	  case BC_DADD:
	    regs.SP[1].as_double += regs.SP[0].as_double;
	    regs.SP++;
	  break;
	  
	  case BC_IADD:
	    regs.SP[1].as_int += regs.SP[0].as_int;
	    regs.SP++;
	  break;
	  case BC_DSUB:
	    regs.SP[1].as_double -=  regs.SP[0].as_double;
	    regs.SP++;
	  break;
	  case BC_ISUB:
	    regs.SP[1].as_int -=  regs.SP[0].as_int;
	    regs.SP++;
	  break;
	  case BC_DMUL:
	    regs.SP[1].as_double *= regs.SP[0].as_double;
	    regs.SP++;
	  break;
	  case BC_IMUL:
	    regs.SP[1].as_int *= regs.SP[0].as_int;
	    regs.SP++;
	  break;
	  case BC_DDIV:
	    regs.SP[1].as_double /= regs.SP[0].as_double;
	    regs.SP++;
	  break;
	  case BC_IDIV:
	    regs.SP[1].as_int /= regs.SP[0].as_int;
	    regs.SP++;
	  break;
	  case BC_IMOD:
	    regs.SP[1].as_int %= regs.SP[0].as_int;
	    regs.SP++;
	  break;
	  case BC_DNEG:
	    regs.SP[0].as_double = - regs.SP[0].as_double;
	  break;	
	  case BC_INEG:
	    regs.SP[0].as_int = - regs.SP[0].as_int;
	  break;
	  case BC_IPRINT:
	    printf( "%ld", regs.SP[0].as_int );
	    regs.SP++;
	  break;
	  case BC_DPRINT:
	    printf( "%lf", regs.SP[0].as_double );
	    regs.SP++;
	  break;
	  case BC_SPRINT:
	    fputs( regs.SP[0].as_ptr , stdout );
	    regs.SP++;
	  break;
	  case BC_I2D:
	    regs.SP[0].as_double = (double) ( regs.SP[0].as_int );
	  break;	
	  case BC_D2I:
	  regs.SP[0].as_int = (int) ( regs.SP[0].as_double );
	  break;
	  case BC_S2I:
	    regs.SP[0].as_int = atol( regs.SP[0].as_ptr );    
	  break;
	  case BC_SWAP:
	  {
	    cell_t temp_cell;
	    temp_cell = regs.SP[0];
	    regs.SP[0] = regs.SP[1];
	    regs.SP[1] = temp_cell;
	  }; break;
	  case BC_POP:
	    regs.SP ++;
	  break; 
	   
	  
	  case BC_LOADIVAR0:
	    case BC_LOADSVAR0:
	      case BC_LOADDVAR0:
	    regs.SP--;
	    regs.SP[0] = g_ctx_tos->locals[0];
	  break;
	
	  case BC_LOADIVAR1:
	    case BC_LOADSVAR1:
	      case BC_LOADDVAR1:
	    regs.SP--;
	    regs.SP[0] = g_ctx_tos->locals[1];
	  break;
	  case BC_LOADIVAR2:
	    case BC_LOADSVAR2:
	      case BC_LOADDVAR2:
	    regs.SP--;
	    regs.SP[0] = g_ctx_tos->locals[2];
	  break;
	  
	  case BC_LOADIVAR3:
	    case BC_LOADSVAR3:
	      case BC_LOADDVAR3:
	    regs.SP--;
	    regs.SP[0] = g_ctx_tos->locals[3];
	  break;
	  
	  case BC_STOREIVAR0:
	    case BC_STORESVAR0:
	      case BC_STOREDVAR0:
	    g_ctx_tos->locals[0] = regs.SP[0];
	    ++regs.SP;
	  break;	  
	  
	  case BC_STOREIVAR1:
	    case BC_STORESVAR1:
	      case BC_STOREDVAR1:
	    g_ctx_tos->locals[1] = regs.SP[0];
	    ++regs.SP;
	  break;
	  
	  case BC_STOREIVAR2:
	    case BC_STORESVAR2:
	      case BC_STOREDVAR2:
	    g_ctx_tos->locals[2] = regs.SP[0];
	    ++regs.SP;
	  break;
	  
	  case BC_STOREIVAR3:
	    case BC_STORESVAR3:
	      case BC_STOREDVAR3: 
	    g_ctx_tos->locals[3] = regs.SP[0];
	    ++regs.SP;
	  break;
	  
	  case BC_LOADDVAR:
	    case BC_LOADIVAR:
	      case BC_LOADSVAR:
	    regs.SP--;
	    regs.SP[0] = g_ctx_tos->locals[GET_WORD(regs.IP)];
	    regs.IP += sizeof( uint16_t );
	  break;
	  
	  case BC_STOREIVAR:
	  case BC_STOREDVAR:
	  case BC_STORESVAR:
	    g_ctx_tos-> locals[*((uint16_t *) (regs.IP))].as_double = regs.SP[0].as_double;
	    regs.IP += sizeof( uint16_t );
	    regs.SP++;
	  break;
	  
	  case BC_LOADCTXSVAR:
	  case BC_LOADCTXIVAR:
	  case BC_LOADCTXDVAR:
	  {
	    const vm_id_t ctx = * ( (vm_id_t*)(regs.IP) );
	    const uint16_t  loc_idx = * ( (vm_id_t*)(regs.IP + 2) );
	    
	    regs.SP[-1] = table_get( ctx )-> topmost_runtime_ctx-> locals[ loc_idx ];
	    regs.IP += sizeof(vm_id_t) + sizeof(uint16_t );
	    
	    regs.SP --;
	  }
	  break;
	  
	  
	  case BC_STORECTXDVAR:
	  case BC_STORECTXSVAR:
	  case BC_STORECTXIVAR:
	  {
	    const vm_id_t ctx = GET_WORD(regs.IP);
	    const uint16_t  loc_idx = GET_WORD(regs.IP + 2);
	    
	    table_get( ctx )-> topmost_runtime_ctx-> locals[ loc_idx ].as_double = regs.SP[0].as_double;
	    regs.IP += sizeof(vm_id_t) + sizeof(uint16_t );
	    
	    regs.SP ++;
	  }
	  break;
	  
	  case BC_DCMP:
	  { 
	    if ( regs.SP[0].as_double > regs.SP[1].as_double )
	      regs.SP[1].as_int = 1;
	    else if ( regs.SP[0].as_double < regs.SP[1].as_double )
	      regs.SP[1].as_int = -1;
	    else regs.SP[1].as_int = 0;
	    regs.SP++;
	    
	  }
	  break;
	  case BC_ICMP: 
	  {
	    const uint64_t fst = regs.SP[0].as_int;
	    const uint64_t snd = regs.SP[1].as_int;
	    //printf(" Comparing %ld with %ld \n", fst, snd );
	    if (fst > snd )
	      regs.SP[1].as_int = 1;
	    else if ( fst < snd )
	      regs.SP[1].as_int = -1;
	    else regs.SP[1].as_int = 0;    
	    regs.SP++;
	  }
	  break;
	  case BC_JA:
	  {
	    const int16_t  offset = GET_WORD( regs.IP );
	    regs.IP += offset;
	  }
	  break;
	  case BC_IFICMPNE:
	  {
	    const int16_t  offset = GET_WORD( regs.IP );
	    
	    //printf(" Comparing %ld with %ld \n", g_registers.SP[0].as_int,  g_registers.SP[1].as_int);
	    if ( regs.SP[1].as_int != regs.SP[0].as_int )
	      regs.IP += offset;
	    else regs.IP += sizeof( uint16_t  );
	    regs.SP += 2;
	  }
	  break;
	  
	  case BC_IFICMPE:
	  {
	    const int16_t  offset = GET_WORD( regs.IP );
	    //printf(" Comparing %ld with %ld , offset %d \n", g_registers.SP[0].as_int,  g_registers.SP[1].as_int, offset );
	    if ( regs.SP[1].as_int == regs.SP[0].as_int )
	      regs.IP += offset;
	    else regs.IP += sizeof( uint16_t  );
	    regs.SP += 2;
	  }
	  break;
	  
	  case BC_IFICMPG:
	  {
	    const int16_t  offset = GET_WORD( regs.IP );
	    //printf(" Comparing %ld with %ld \n", g_registers.SP[0].as_int,  g_registers.SP[1].as_int);
	    if ( regs.SP[1].as_int > regs.SP[0].as_int )
	      regs.IP += offset;
	    else regs.IP += sizeof( uint16_t  );
	    regs.SP += 2;
	  };break;
	  
	  case BC_IFICMPGE:
	  {
	    const int16_t  offset = GET_WORD( regs.IP );
	    //printf(" Comparing %ld with %ld \n", g_registers.SP[0].as_int,  g_registers.SP[1].as_int);
	    if ( regs.SP[1].as_int >= regs.SP[0].as_int )
	      regs.IP += offset;
	    else regs.IP += sizeof( uint16_t  );
	    regs.SP += 2;
	  }; break;
	  
	  case BC_IFICMPL:
	  {
	    const int16_t  offset = GET_WORD( regs.IP );
	    //printf(" Comparing %ld with %ld \n", g_registers.SP[0].as_int,  g_registers.SP[1].as_int);
	    if ( regs.SP[1].as_int < regs.SP[0].as_int )
	      regs.IP += offset;
	    else regs.IP += sizeof( uint16_t  );
	    regs.SP += 2;
	  }
	  break;
	  
	  case BC_IFICMPLE:
	  {
	    const int16_t  offset = GET_WORD( regs.IP ); 
	    if ( regs.SP[1].as_int <= regs.SP[0].as_int )
	      regs.IP += offset;
	    else regs.IP += sizeof( uint16_t  );
	    regs.SP += 2;
	  }
	  break;
	  
	  case BC_DUMP:
	    printf( "%ld %lf %p\n", regs.SP[0].as_int, regs.SP[0].as_double,  regs.SP[0].as_ptr );
	  break;
	  
	  case BC_STOP:
	    longjmp( stop_label, 0 );
	  break;
	  case BC_CALL:
	  {
	    const vm_id_t id = GET_WORD( regs.IP );
	    
	    const function_t* func = table_get( id );
	    const ctx_static_t* ctx = func-> static_ctx;
	    
	    ctx_push( ctx, regs.IP + sizeof (uint16_t ) );
	    regs.IP = func-> code;
	    memcpy( func->topmost_runtime_ctx->locals, regs.SP, func->static_ctx->locals_size );
	    regs.SP += func->static_ctx->locals_size /8; 
	  };
	  break;
	  
	  case BC_CALLNATIVE:
	    fprintf( stderr, "Native calls are not implemented yet");
	  break;
	  
	  case BC_RETURN:
	    regs.IP = g_ctx_tos-> return_address;  
	    ctx_pop();
	  break;
	  
	  case BC_BREAK:
	  break;
	  
	  case BC_IAOR:
	    regs.SP[1].as_int |= regs.SP[0].as_int;
	    regs.SP++; 
	  break;
	  
	  case BC_IAAND:
	    regs.SP[1].as_int &= regs.SP[0].as_int;
	    regs.SP++;
	  break;
	  
	  
	  case BC_IAXOR:
	    regs.SP[1].as_int ^= regs.SP[0].as_int;
	    regs.SP++;
	  break;	  
	} 
	
      }
    }
    
  }

  }
