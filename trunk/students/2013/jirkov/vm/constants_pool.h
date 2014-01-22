#ifndef CONSTANTS_POOL_H
#define CONSTANTS_POOL_H

#include <stdlib.h>

#include "common.h"

namespace mathvm {
void constants_pool_init( size_t initial_capacity, size_t count );
void constants_pool_add( const char_t* const str );
const char_t* const constant_pool_get( const int16_t id );
void constants_pool_deinit( void );
}
#endif