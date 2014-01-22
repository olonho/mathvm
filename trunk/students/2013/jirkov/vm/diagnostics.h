#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdio.h>
#include "function.h" 
#include "context.h"
#include "hash_table.h"
#include "instruction_set.h"
namespace mathvm {
  
void bytecode_dump( const byte_t* bc, FILE* file );

size_t instruction_dump( const byte_t* const bytes, FILE* file );

void function_dump( const function_t* const fun, FILE* file);

void ctx_static_dump( const ctx_static_t* const ctx, FILE* file );

void table_dump( FILE* file );

void registers_dump( const registers_t* const regs, FILE* f );

void calc_stack_dump( FILE* file );

}
#endif