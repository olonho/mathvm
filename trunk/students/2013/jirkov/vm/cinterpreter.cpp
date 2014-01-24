#include <iostream>
#include <cstdio>
#include <malloc.h>
#include <setjmp.h>
#include <string.h>

#include "common.h"
#include "context.h"
#include "constants_pool.h"
#include "instruction_set.h"
#include "diagnostics.h"


#include "../../../../include/ast.h"
#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"
#include "../../../../vm/parser.h" 


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
    calc_stack_init( 65536*16*16 );
  }
  
  void interpreter_deinit( void )
  {
    ctx_stack_deinit();
    calc_stack_deinit();
    constants_pool_deinit();
  }
  
  
  
  #define ADDR(b, s, l) &&_BC_##b,
  
  void interpreter_start( byte_t* start )
  {
    //     volatile bool_t launched = false; 
    registers_t regs;
    
    static  void*  insns[] = { FOR_BYTECODES( ADDR ) &&_BC_INVALID };
    regs.IP = start;
    regs.SP = g_calc_stack + g_calc_stack_count-1;
    
    #define NEXT goto *(insns[*regs.IP++]);
    NEXT 
    
    _BC_INVALID: 
    NEXT
  
    _BC_SLOAD:
    regs.SP--;
    regs.SP->as_ptr = (char*)( constant_pool_get( GET_WORD( regs.IP ) ) );
    regs.IP += sizeof( uint16_t );
    NEXT
    
    _BC_DLOAD:
    _BC_ILOAD:
    regs.SP--;
    regs.SP[0] = GET_CELL( regs.IP );
    regs.IP += sizeof( cell_t );
    NEXT
    
    
    _BC_SLOAD0:
    regs.SP--;
    regs.SP-> as_ptr = (char*) constant_pool_get( 0 );
    NEXT
    _BC_ILOAD0:
    _BC_DLOAD0:
    regs.SP--;
    regs.SP-> as_int = 0;
    NEXT 
    _BC_DLOAD1:
    regs.SP--;
    regs.SP-> as_double = 1.0;
    NEXT
    _BC_ILOAD1:
    regs.SP--;
    regs.SP-> as_int = 1;
    NEXT
    _BC_DLOADM1:
    regs.SP--;
    regs.SP-> as_double = -1.0;
    NEXT
    
    _BC_ILOADM1:
    regs.SP--;
    regs.SP-> as_int = -1;
    NEXT
    
    _BC_DADD:
    regs.SP[1].as_double += regs.SP[0].as_double;
    regs.SP++;
    NEXT
    
    _BC_IADD:
    regs.SP[1].as_int += regs.SP[0].as_int;
    regs.SP++;
    NEXT
    _BC_DSUB:
    regs.SP[1].as_double -=  regs.SP[0].as_double;
    regs.SP++;
    NEXT
    _BC_ISUB:
    regs.SP[1].as_int -=  regs.SP[0].as_int;
    regs.SP++;
    NEXT
    _BC_DMUL:
    regs.SP[1].as_double *= regs.SP[0].as_double;
    regs.SP++;
    NEXT
    _BC_IMUL:
    regs.SP[1].as_int *= regs.SP[0].as_int;
    regs.SP++;
    NEXT
    _BC_DDIV:
    regs.SP[1].as_double /= regs.SP[0].as_double;
    regs.SP++;
    NEXT
    _BC_IDIV:
    regs.SP[1].as_int /= regs.SP[0].as_int;
    regs.SP++;
    NEXT
    _BC_IMOD:
    regs.SP[1].as_int %= regs.SP[0].as_int;
    regs.SP++;
    NEXT
    _BC_DNEG:
    regs.SP[0].as_double = - regs.SP[0].as_double;
    NEXT	
    _BC_INEG:
    regs.SP[0].as_int = - regs.SP[0].as_int;
    NEXT
    _BC_IPRINT:
    std::cout << regs.SP[0].as_int ;
    regs.SP++;
    NEXT
    _BC_DPRINT:
    std::cout << regs.SP[0].as_double;
    regs.SP++;
    NEXT
    _BC_SPRINT:
    std::cout << regs.SP[0].as_ptr;
    regs.SP++;
    NEXT
    _BC_I2D:
    regs.SP[0].as_double = (double) ( regs.SP[0].as_int );
    NEXT	
    _BC_D2I:
    regs.SP[0].as_int = (int) ( regs.SP[0].as_double );
    NEXT
    _BC_S2I:
    regs.SP[0].as_int = atol( regs.SP[0].as_ptr );    
    NEXT
    _BC_SWAP:
    {
      cell_t temp_cell;
      temp_cell = regs.SP[0];
      regs.SP[0] = regs.SP[1];
      regs.SP[1] = temp_cell;
    }; NEXT
    _BC_POP:
    regs.SP ++;
    NEXT 
    
    
    _BC_LOADIVAR0:
    _BC_LOADSVAR0:
    _BC_LOADDVAR0:
    regs.SP--;
    regs.SP[0] = g_ctx_tos->locals[0];
    NEXT
    
    _BC_LOADIVAR1:
    _BC_LOADSVAR1:
    _BC_LOADDVAR1:
    regs.SP--;
    regs.SP[0] = g_ctx_tos->locals[1];
    NEXT
    _BC_LOADIVAR2:
    _BC_LOADSVAR2:
    _BC_LOADDVAR2:
    regs.SP--;
    regs.SP[0] = g_ctx_tos->locals[2];
    NEXT
    
    _BC_LOADIVAR3:
    _BC_LOADSVAR3:
    _BC_LOADDVAR3:
    regs.SP--;
    regs.SP[0] = g_ctx_tos->locals[3];
    NEXT
    
    _BC_STOREIVAR0:
    _BC_STORESVAR0:
    _BC_STOREDVAR0:
    g_ctx_tos->locals[0] = regs.SP[0];
    ++regs.SP;
    NEXT	  
    
    _BC_STOREIVAR1:
    _BC_STORESVAR1:
    _BC_STOREDVAR1:
    g_ctx_tos->locals[1] = regs.SP[0];
    ++regs.SP;
    NEXT
    
    _BC_STOREIVAR2:
    _BC_STORESVAR2:
    _BC_STOREDVAR2:
    g_ctx_tos->locals[2] = regs.SP[0];
    ++regs.SP;
    NEXT
    
    _BC_STOREIVAR3:
    _BC_STORESVAR3:
    _BC_STOREDVAR3: 
    g_ctx_tos->locals[3] = regs.SP[0];
    ++regs.SP;
    NEXT
    
    _BC_LOADDVAR:
    _BC_LOADIVAR:
    _BC_LOADSVAR:
    regs.SP--;
    regs.SP[0] = g_ctx_tos->locals[GET_WORD(regs.IP)];
    regs.IP += sizeof( uint16_t );
    NEXT
    
    _BC_STOREIVAR:
    _BC_STOREDVAR:
    _BC_STORESVAR:
    g_ctx_tos-> locals[*((uint16_t *) (regs.IP))].as_double = regs.SP[0].as_double;
    regs.IP += sizeof( uint16_t );
    regs.SP++;
    NEXT
    
    _BC_LOADCTXSVAR:
    _BC_LOADCTXIVAR:
    _BC_LOADCTXDVAR:
    {
      const vm_id_t ctx = * ( (vm_id_t*)(regs.IP) );
      const uint16_t  loc_idx = * ( (vm_id_t*)(regs.IP + 2) );
      
      regs.SP[-1] = table_get( ctx )-> topmost_runtime_ctx-> locals[ loc_idx ];
      regs.IP += sizeof(vm_id_t) + sizeof(uint16_t );
      
      regs.SP --;
    }
    NEXT
    
    
    _BC_STORECTXDVAR:
    _BC_STORECTXSVAR:
    _BC_STORECTXIVAR:
    {
      const vm_id_t ctx = GET_WORD(regs.IP);
      const uint16_t  loc_idx = GET_WORD(regs.IP + 2);
      
      table_get( ctx )-> topmost_runtime_ctx-> locals[ loc_idx ].as_double = regs.SP[0].as_double;
      regs.IP += sizeof(vm_id_t) + sizeof(uint16_t );
      
      regs.SP ++;
    }
    NEXT
    
    _BC_DCMP:
    { 
      if ( regs.SP[0].as_double > regs.SP[1].as_double )
	regs.SP[1].as_int = 1;
      else if ( regs.SP[0].as_double < regs.SP[1].as_double )
	regs.SP[1].as_int = -1;
      else regs.SP[1].as_int = 0;
      regs.SP++;
      
    }
    NEXT
    _BC_ICMP: 
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
    NEXT
    _BC_JA:
    {
      const int16_t  offset = GET_WORD( regs.IP );
      regs.IP += offset;
    }
    NEXT
    _BC_IFICMPNE:
    {
      const int16_t  offset = GET_WORD( regs.IP );
      
      //printf(" Comparing %ld with %ld \n", g_registers.SP[0].as_int,  g_registers.SP[1].as_int);
      if ( regs.SP[1].as_int != regs.SP[0].as_int )
	regs.IP += offset;
      else regs.IP += sizeof( uint16_t  );
      regs.SP += 2;
    }
    NEXT
    
    _BC_IFICMPE:
    {
      const int16_t  offset = GET_WORD( regs.IP );
      //printf(" Comparing %ld with %ld , offset %d \n", g_registers.SP[0].as_int,  g_registers.SP[1].as_int, offset );
      if ( regs.SP[1].as_int == regs.SP[0].as_int )
	regs.IP += offset;
      else regs.IP += sizeof( uint16_t  );
      regs.SP += 2;
    }
    NEXT
    
    _BC_IFICMPG:
    {
      const int16_t  offset = GET_WORD( regs.IP );
      //printf(" Comparing %ld with %ld \n", g_registers.SP[0].as_int,  g_registers.SP[1].as_int);
      if ( regs.SP[1].as_int > regs.SP[0].as_int )
	regs.IP += offset;
      else regs.IP += sizeof( uint16_t  );
      regs.SP += 2;
    };NEXT
    
    _BC_IFICMPGE:
    {
      const int16_t  offset = GET_WORD( regs.IP );
      //printf(" Comparing %ld with %ld \n", g_registers.SP[0].as_int,  g_registers.SP[1].as_int);
      if ( regs.SP[1].as_int >= regs.SP[0].as_int )
	regs.IP += offset;
      else regs.IP += sizeof( uint16_t  );
      regs.SP += 2;
    }; NEXT
    
    _BC_IFICMPL:
    {
      const int16_t  offset = GET_WORD( regs.IP );
      //printf(" Comparing %ld with %ld \n", g_registers.SP[0].as_int,  g_registers.SP[1].as_int);
      if ( regs.SP[1].as_int < regs.SP[0].as_int )
	regs.IP += offset;
      else regs.IP += sizeof( uint16_t  );
      regs.SP += 2;
      NEXT
    }
    
    _BC_IFICMPLE:
    {
      const int16_t  offset = GET_WORD( regs.IP ); 
      if ( regs.SP[1].as_int <= regs.SP[0].as_int )
	regs.IP += offset;
      else regs.IP += sizeof( uint16_t  );
      regs.SP += 2;
      NEXT
    }
    
    _BC_DUMP:
    std::cout << regs.SP[0].as_int << " " << regs.SP[0].as_double << " " << regs.SP[0].as_ptr << std::endl; 
    NEXT
    
    
    _BC_CALL:
    {
      const vm_id_t id = GET_WORD( regs.IP );
      
      const function_t* func = table_get( id );
      const ctx_static_t* ctx = func-> static_ctx;
      
//       std::cout<< "calling function " << func->id << std::endl;
      ctx_push( ctx, regs.IP + sizeof (uint16_t ) );
      regs.IP = func-> code;
      memcpy( func->topmost_runtime_ctx->locals, regs.SP, func->static_ctx->locals_size );
      regs.SP += func->static_ctx->locals_size /8; 
    };
    NEXT
    
    _BC_CALLNATIVE:
    std::cerr<< "Native calls are not implemented yet" ;
    NEXT
    
    _BC_RETURN:
    regs.IP = g_ctx_tos-> return_address;  
    ctx_pop();
    NEXT
    
    _BC_BREAK:
    NEXT
    
    _BC_IAOR:
    regs.SP[1].as_int |= regs.SP[0].as_int;
    regs.SP++; 
    NEXT
    
    _BC_IAAND:
    regs.SP[1].as_int &= regs.SP[0].as_int;
    regs.SP++;
    NEXT
    
    
    _BC_IAXOR:
    regs.SP[1].as_int ^= regs.SP[0].as_int;
    regs.SP++;
    
    
    
    _BC_STOP:  {}

    
  }




}
