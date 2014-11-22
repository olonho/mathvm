#ifndef __LOGGER_HPP_
#define __LOGGER_HPP_

#include <iostream>

#ifdef DEBUG
    #define LOG(message) std::cout << message << endl;
    #define LOG_Visitor(message)
//    #define LOG_INTERPRETER
#else
    #define LOG(message)
    #define LOG_Visitor(message)
#endif

#endif