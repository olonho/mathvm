#if defined(_MSC_VER)

#define strtoll _strtoi64
#pragma warning(disable : 4996)
#pragma warning(disable : 4244)

#include <io.h>

#endif

