#ifndef PLACEHOLDER_H
#define PLACEHOLDER_H

#include <inttypes.h>

union PlaceHolder{
    int64_t i;
    double d;
    PlaceHolder(int64_t n):i(n){}
    PlaceHolder(double n):d(n){}

};

#endif // PLACEHOLDER_H
