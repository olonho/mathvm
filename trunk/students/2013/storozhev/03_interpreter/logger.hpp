#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <sstream>

#ifdef DEBUG
#define LOGGER std::cout
#else
#define LOGGER std::stringstream()
#endif

#endif // LOGGER_HPP
