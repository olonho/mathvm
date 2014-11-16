#ifndef __LOGGER_HPP_
#define __LOGGER_HPP_

#include <sstream>
#include <iostream>

static std::stringstream devnull;

#ifdef DEBUG
#define LOG std::cout
#else
#define LOG devnull
#endif

#endif