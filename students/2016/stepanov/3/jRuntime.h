#ifndef VM_AF_3_JRUNTIME_H
#define VM_AF_3_JRUNTIME_H


#include <cstdio>
#include <cstdint>
#include <map>

int64_t print_double(double x);

int64_t print_int(int64_t x);

int64_t print_string(int8_t *x);

int64_t idiv_div_int(int64_t x, int64_t y);

int64_t idiv_mod_int(int64_t x, int64_t y);

int64_t ucomisd_le(double x, double y);
int64_t ucomisd_l(double x, double y);
int64_t ucomisd_g(double x, double y);
int64_t ucomisd_ge(double x, double y);
int64_t ucomisd_e(double x, double y);
int64_t ucomisd_ne(double x, double y);

extern char * constantPool[UINT16_MAX];

extern int constantPoolId;

extern std::map<std::string, int> constantMap;

#endif //VM_AF_3_JRUNTIME_H
