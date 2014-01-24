#ifndef CONTEXT_H
#define CONTEXT_H

#include "common.h"
#include "cinterpreter.h"

namespace mathvm {
typedef unsigned short vm_id_t;

/* The size is 
sizeof(ctx_static_t) iff is_zero_initialized is TRUE
OTHERWISE sizeof(ctx_static_t) + locals_size
*/

typedef struct {
	vm_id_t id;
	bool_t is_zero_initialized;
	size_t stack_size;
	size_t locals_size;
	cell_t locals_defaults[];
} ctx_static_t;

typedef struct ctx_runtime_t {
	struct ctx_runtime_t* previous_called;
	struct ctx_runtime_t* previous_same_id;
	const byte_t* return_address;
	const ctx_static_t* description;
	cell_t locals[];
} ctx_runtime_t;;



ctx_static_t* ctx_create_and_bind( 
	const vm_id_t id,  
	const bool_t is_zero_initialized,
	const size_t stack_size,
	const size_t locals_size,
	const void* const locals_defaults
	);

void ctx_destroy( ctx_static_t* ctx );

void ctx_stack_init( const size_t size );

void ctx_stack_deinit( void );

/* returns the new tos */
ctx_runtime_t* ctx_push( const ctx_static_t* const info, const byte_t* const return_address );


/* returns the static_ctx on tos */
// ctx_runtime_t* get_topmost_ctx( void );
extern ctx_runtime_t* g_ctx_tos;

/* returns the last static_ctx with given ID */
ctx_runtime_t* get_topmost_ctx_of_id( const vm_id_t id ); 

/* returns the new tos or NULL if reached the root static_ctx */
ctx_runtime_t* ctx_pop( void );


}
#endif
