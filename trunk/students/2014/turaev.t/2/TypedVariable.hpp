#ifndef __TypedVariable_HPP_
#define __TypedVariable_HPP_

#include "mathvm.h"

namespace mathvm {
    class TypedVariable {
        union {
            double _doubleValue;
            int64_t _intValue;
            const char *_stringValue;
        };

    public:
        TypedVariable(VarType type) : _type(type) {
            switch (type) {
                case VT_DOUBLE:
                    setDoubleValue(0.0);
                    break;
                case VT_INT:
                    setIntValue(0);
                    break;
                case VT_STRING:
                    setStringValue(0);
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