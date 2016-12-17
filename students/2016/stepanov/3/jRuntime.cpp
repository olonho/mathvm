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

int64_t ucomisd_le(double x, double y) {
    return  x <= y;
}
int64_t ucomisd_l(double x, double y) {
    return  x < y;
}

int64_t ucomisd_g(double x, double y) {
    return  x > y;
}

int64_t ucomisd_ge(double x, double y) {
    return  x >= y;
}

int64_t ucomisd_e(double x, double y) {
    return  x == y;
}

int64_t ucomisd_ne(double x, double y) {
    return  x != y;
}
std::map<std::string, int> constantMap;