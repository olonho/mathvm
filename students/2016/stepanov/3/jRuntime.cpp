#include <iostream>
#include "jRuntime.h"

int64_t print_double(double x) {
    std::cout << x;
    return 0;
}

int64_t print_int(int64_t x) {
    std::cout << x;
    return 0;
}

int64_t print_string(int8_t *x) {
    std::cout << (char *)x;
    return 0;
}



/*
 * asmjit compiler has problem with idiv
 */

int64_t idiv_div_int(int64_t x, int64_t y) {
    return x / y;
}

int64_t idiv_mod_int(int64_t x, int64_t y) {
    return x % y;
}

char * constantPool[UINT16_MAX];


int constantPoolId = 0;

std::map<std::string, int> constantMap;