#ifndef __DEBUG_MACROS_H__
#define __DEBUT_MACROS_H__

#ifdef DEBUG

#include <iostream>

#define LOG(WHAT) std::cerr << WHAT << std::endl

#else

#define LOG(WHAT) 

#endif

#endif
