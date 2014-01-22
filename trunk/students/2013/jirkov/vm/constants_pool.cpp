#include <malloc.h>
#include <string.h>
#include "constants_pool.h"

namespace mathvm {
static char_t* constants_pool = NULL;
static char_t** constants_pointers = NULL;

static size_t next_const_idx = 0;
static size_t next_const_ptr_idx = 0;


void constants_pool_init( size_t initial_capacity, size_t count )
{
	constants_pool = (char_t*) calloc( initial_capacity, sizeof( char_t ));
	constants_pointers = (char_t**) calloc( count, sizeof( char_t* ) );
}

void constants_pool_add( const char_t* const str )
{
	size_t length;

	length = strlen( str );
	strcpy( constants_pool + next_const_idx, str );

	constants_pointers[next_const_ptr_idx] = constants_pool + next_const_idx;
	next_const_idx += length + 1;
	next_const_ptr_idx ++;
}

const char_t* const constant_pool_get( const int16_t id )
{
	return constants_pointers[ id ];
}

void constants_pool_deinit( void )
{
	free( constants_pool );
	free( constants_pointers );
}
}