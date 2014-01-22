#ifndef FUNCTION_H
#define FUNCTION_H

#include "context.h"

namespace mathvm {
typedef struct {  
	vm_id_t id;
	const char* signature;
	const ctx_static_t* static_ctx;
	ctx_runtime_t* topmost_runtime_ctx;
	size_t code_size;
	byte_t code[]; //flexible arrays, yay!
} function_t;

function_t* function_create( 
	const vm_id_t id,
	const char* signature, 
	const size_t code_size, 
	const byte_t* code);

void function_destroy( const vm_id_t id );


}
#endif