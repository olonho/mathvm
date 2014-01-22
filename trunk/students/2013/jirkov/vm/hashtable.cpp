#include <ctype.h>
#include <stdlib.h>
#include <stdio.h> 

#include "hash_table.h"
#include "function.h"

namespace mathvm {
#define HASH_MULTIPLIER 29

vm_id_t hash( const char* str )
{
	vm_id_t acc = 0;

	for(;;)
	{
		switch( *str )
		{
		case '\0': return acc;
		case '(':
		case ')':
		case ' ': continue;
		default:
			acc = (vm_id_t)(acc * HASH_MULTIPLIER + tolower(*str) );
			break;
		}
	}
}

function_t* hash_table[HASH_TABLE_SIZE] = { NULL };

function_t* const table_get( vm_id_t hash ) 
{
	function_t** candidate = hash_table + hash;

	do {
		if (*candidate == NULL) return NULL;
		if ((**candidate).id == hash ) return *candidate;
	}
	while( candidate != hash_table + HASH_TABLE_SIZE );
	return NULL;
}

// static function_t** const table_get_pointer( vm_id_t hash )
// {
// 	function_t** candidate = hash_table + hash;
// 
// 	do {
// 		if (*candidate == NULL) return NULL;
// 		if ((**candidate).id == hash ) return candidate;
// 	}
// 	while( candidate != hash_table + HASH_TABLE_SIZE );
// 	return NULL;
// }

void table_put( vm_id_t hash, function_t* const header )
{
	function_t** where = hash_table + hash;
	while( *where != NULL && where < hash_table + HASH_TABLE_SIZE ) 
	where++;

	if ( *where ==  NULL ) 
		(*where) = header;
	else
		fprintf( stderr, "No place to put function with id %x!", hash );
}

function_t* table_remove( const vm_id_t hash )
{
	function_t** where = hash_table + hash;
	function_t* result = NULL;

	while( *where != NULL && where < hash_table + HASH_TABLE_SIZE ) 
	{
		if ( (**where).id == hash )
		{
			result = *where;
			*where = NULL;
			return result;
		}
		where++;
	}

	fprintf( stderr, "Nothing to remove. id %x", hash );
	return NULL;
}

}