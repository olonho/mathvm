//
// Created by andy on 11/23/16.
//

#ifndef PROJECT_UTIL_H
#define PROJECT_UTIL_H

#include "mathvm.h"

namespace mathvm
{
// it's like Var, but without string SOO memory overhead
class AnonymousVar
{
public:
    VarType type;
    union
    {
        double doubleValue;
        int64_t intValue;
        const char *stringValue;
    };

    AnonymousVar(VarType type) : type(type) {
        switch (type) {
            case VT_INT:
                intValue = 0;
                break;
            case VT_DOUBLE:
                doubleValue = 0;
                break;
            case VT_STRING:
                stringValue = 0;
                break;
            default:
                assert(false);
        }
    }
    AnonymousVar(double value) : type(VT_DOUBLE), doubleValue(value) { }
    AnonymousVar(int64_t value)  : type(VT_INT), intValue(value) { }
    AnonymousVar(const char *value) : type(VT_STRING), stringValue(value) { }

    double getDoubleValue() const {
        assert(type == VT_DOUBLE);
        return doubleValue;
    }

    void setIntValue(int64_t value) {
        assert(type == VT_INT);
        intValue = value;
    }

    void setDoubleValue(double value) {
        assert(type == VT_DOUBLE);
        doubleValue = value;
    }

    int64_t getIntValue() const {
        assert(type == VT_INT);
        return intValue;
    }

    void setStringValue(const char* value) {
        assert(type == VT_STRING);
        stringValue = value;
    }

    const char* getStringValue() const {
        assert(type == VT_STRING);
        return stringValue;
    }

    AnonymousVar& operator=(const Var&v) {
        assert(type == v.type());
        switch (type) {
            case VT_DOUBLE:
                doubleValue = v.getDoubleValue();
                break;
            case VT_INT:
                intValue = v.getIntValue();
                break;
            case VT_STRING:
                stringValue = v.getStringValue();
                break;
            default:
                assert(false);
        }

        return *this;
    }
};
}
#endif //PROJECT_UTIL_H
