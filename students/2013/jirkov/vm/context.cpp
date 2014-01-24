#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "common.h"
#include "context.h"
#include "hash_table.h"

#define RESERVE_STACK_SPACE( size ) g_ctx_tos = (ctx_runtime_t*) (( (byte_t*) g_ctx_tos ) - (size))
#define REWIND_STACK( size ) g_ctx_tos = (ctx_runtime_t*) (( (byte_t*) g_ctx_tos ) + (size))
#define ENOUGH_SPACE_IN_STACK( size ) (((byte_t*)g_ctx_tos - size) >= (byte_t*)g_ctx_stack)


namespace mathvm {
/* Globals */

void* g_ctx_stack = NULL;
size_t g_ctx_stack_size = 0;

//ctx_runtime_t* g_ctx_root_runtime = NULL;
//de
//ctx_static_t g_ctx_root_info = {
//	0,
//	TRUE,
//	0,
//	0
//};

ctx_runtime_t* g_ctx_tos = NULL;

/* Functions */

void ctx_stack_init( const size_t size )
{
	g_ctx_stack = malloc( size );
	g_ctx_stack_size = size;
	g_ctx_tos = (ctx_runtime_t*) ((byte_t*) g_ctx_stack + size-1 );
	//g_ctx_root_runtime = ctx_push( & g_ctx_root_info );

}

void ctx_stack_deinit( void )
{
	free( g_ctx_stack );
}



/* returns the static_ctx on tos */
ctx_runtime_t* get_topmost_ctx( void ) 
{
	return g_ctx_tos;
}

/* returns the last static_ctx with given ID */
ctx_runtime_t* get_topmost_ctx_of_id( const vm_id_t id ) {
	function_t* fun;
	fun = table_get( id );
	if ( fun == NULL ) return NULL;

	return fun-> topmost_runtime_ctx;
}

ctx_static_t* ctx_create_and_bind( 
	const vm_id_t id,  
	const bool_t is_zero_initialized,
	const size_t stack_size,
	const size_t locals_size,
	const void* const locals_defaults
	)
{
	ctx_static_t* result;
	size_t ctx_size;

	ctx_size = sizeof( ctx_static_t ) + ( is_zero_initialized? 0 :locals_size );

	result = (ctx_static_t*) malloc( ctx_size );
	result-> id = id;
	result-> is_zero_initialized = is_zero_initialized;
	result-> locals_size = locals_size;
	result-> stack_size = stack_size;
	if (! is_zero_initialized )
		memcpy(result-> locals_defaults, locals_defaults, locals_size );

	table_get( id )-> static_ctx = result;
	return result;
}

void ctx_destroy( ctx_static_t* ctx )
{
	free( ctx );
}

ctx_runtime_t* ctx_push( const ctx_static_t* const info, const byte_t* const return_address )
{
	size_t new_ctx_size;
	ctx_runtime_t* const old_tos = g_ctx_tos;
	function_t* corresponding_function;

	corresponding_function = table_get( info->id );
/*
#ifndef DISABLE_RUNTIME_CHECKS
	if ( corresponding_function == NULL )
	{
		fprintf( stderr, "Tried to make static_ctx without corresponding function! Id: %x\n", info->id );
		return g_ctx_tos;
	}
#endif
*/
	new_ctx_size = info-> locals_size + sizeof( ctx_runtime_t );
/*
#ifndef DISABLE_RUNTIME_CHECKS
	if (! ENOUGH_SPACE_IN_STACK( new_ctx_size ) )
	{
		fprintf( stderr, "Not enough space in stack to allocate %lu bytes! Attempt to create static_ctx with id %x", new_ctx_size, info-> id );
	}
#endif */

	RESERVE_STACK_SPACE( new_ctx_size );

	g_ctx_tos-> description = info;
	g_ctx_tos-> previous_called = old_tos;
	g_ctx_tos-> previous_same_id = corresponding_function-> topmost_runtime_ctx;

	g_ctx_tos-> return_address = return_address;
// 	if( ! info->is_zero_initialized	)
// 		memcpy( g_ctx_tos-> locals, info-> locals_defaults, info-> locals_size );
// 	else
// 		memset( g_ctx_tos-> locals, 0, info-> locals_size );

	corresponding_function-> topmost_runtime_ctx = g_ctx_tos;
	return g_ctx_tos;
}

/* returns the new tos or NULL if reached the root static_ctx */
ctx_runtime_t* ctx_pop( void ) {
	function_t* corresponding_function = table_get( g_ctx_tos-> description-> id );
/*
#ifndef DISABLE_RUNTIME_CHECKS
	if ( corresponding_function == NULL )
	{
		fprintf( stderr, "Attempt to pop static_ctx from TOS, but this static_ctx is not assigned to any function! id: %x", tos->description->id );
	}
#endif*/

	corresponding_function-> topmost_runtime_ctx = corresponding_function->topmost_runtime_ctx-> previous_same_id;

	REWIND_STACK( sizeof( ctx_runtime_t) + g_ctx_tos->description->locals_size );

	return g_ctx_tos;
}

}
