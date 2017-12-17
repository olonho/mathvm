#ifndef DATA_H__
#define DATA_H__

#include "../../../../../include/mathvm.h"

union data {
    data() {}

    data(int16_t id_str) : id(id_str) {}

    data(int64_t id) : value_int(id) {}

    data(double id) : value_double(id) {}

    int16_t id;
    int64_t value_int;
    double value_double;
};

#endif
