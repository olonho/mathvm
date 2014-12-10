#ifndef DATA_HOLDER_H__
#define DATA_HOLDER_H__

union DataHolder {
    DataHolder() {}
    DataHolder(int16_t id): stringId(id) {}
    DataHolder(int64_t id): intValue(id) {}
    DataHolder(double id): doubleValue(id) {}

    int16_t stringId;
    int64_t intValue;
    double doubleValue;
};

#endif
