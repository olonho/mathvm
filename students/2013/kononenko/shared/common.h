#pragma once

#if defined(_MSC_VER)
#include <SDKDDKVer.h>
#define strtoll _strtoi64
#endif

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <sstream>
using std::stringstream;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <map>
using std::map;

#include <stdexcept>

#include <stack>

#include <stdint.h>

#define OVERRIDE override
