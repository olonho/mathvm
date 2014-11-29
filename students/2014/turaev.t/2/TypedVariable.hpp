#ifndef __TypedVariable_HPP_
#define __TypedVariable_HPP_

#include "mathvm.h"

namespace mathvm {
    struct TypedVariable {
    private:
        double _doubleValue;
        int64_t _intValue;
        const char *_stringValue;

    public:
        TypedVariable(VarType type = VT_INT) : _type(type) {
            switch (type) {
                case VT_DOUBLE:
                    _doubleValue = 0.0;
                    break;
                case VT_INT:
                    _intValue = 0;
                    break;
                case VT_STRING:
                    _stringValue = NULL;
                    break;
                default:
                    assert(false);
            }
        }

        VarType _type;

        void setDoubleValue(double value) {
            assert(_type == VT_DOUBLE);
            _doubleValue = value;
        }

        double getDoubleValue() const {
            assert(_type == VT_DOUBLE);
            return _doubleValue;
        }

        void setIntValue(int64_t value) {
            assert(_type == VT_INT);
            _intValue = value;
        }

        int64_t getIntValue() const {
            assert(_type == VT_INT);
            return _intValue;
        }

        void setStringValue(const char *value) {
            assert(_type == VT_STRING);
            _stringValue = value;
        }

        const char *getStringValue() const {
            assert(_type == VT_STRING);
            return _stringValue;
        }
    };
}

#endif