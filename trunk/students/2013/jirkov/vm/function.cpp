
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "hash_table.h"
#include "function.h"

namespace mathvm {
/* after that, code can be freed */
function_t* function_create( 
	const vm_id_t id,
	const char* signature, 
	const size_t code_size, 
	const byte_t* code )
{
	function_t* record;

	record = (function_t*) malloc( sizeof( function_t ) + code_size );

	record-> id = id;
	record-> signature = signature;
	record-> topmost_runtime_ctx = NULL;
	record-> static_ctx = NULL;
	record-> code_size = code_size;

	memcpy( record-> code, code, record-> code_size ); 

	return record;
}

void function_destroy( const vm_id_t id )
{
	function_t* header;
	header = table_remove( id );

#ifndef DISABLE_RUNTIME_CHECKS
	if (header == NULL)
	{
		fprintf( stderr, "No function to remove! id: %x", id );
		return;
	}
#endif
	free( (char*)( header-> signature ) );
	//unconst is ok, since we are doing it to destroy context just before destroying function itself
	ctx_destroy( (ctx_static_t*)( header-> static_ctx ) );
	free( header );
}
}