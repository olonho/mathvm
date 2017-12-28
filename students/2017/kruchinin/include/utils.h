#pragma once

#include "mathvm.h"

using namespace mathvm;

namespace details {
    class StackVar {
    private:
        VarType type;

        union {
            int64_t _int;
            double _double;
            uint16_t _uint16;
        };
    public:
        StackVar() {
            StackVar((int64_t) 0);
        }

        explicit StackVar(int64_t value)
        : _int(value)
        {
            type = VT_INT;
        }

        explicit StackVar(double value)
        : _double(value)
        {
            type = VT_DOUBLE;
        }

        explicit StackVar(uint16_t value)
        : _uint16(value)
        {
            type = VT_STRING;
        }

        ~StackVar() {}

        VarType getType() {
            return type;
        }

        int64_t asInt() {
            return _int;
        }

        double asDouble() {
            return _double;
        }

        uint16_t asUInt16() {
            return _uint16;
        }

        void setDoubleValue(double value) {
            _double = value;
        }

        void setIntValue(int64_t value) {
            _int = value;
        }

        void setUInt16Value(uint16_t value) {
            _uint16 = value;
        }
    };

    class StackMem {
    private:
        stack<StackVar> s;

        StackVar poll() {
            assert(!s.empty() && "stack is empty");
            StackVar tmp = s.top();
            s.pop();
            return tmp;
        }
    public:
        int64_t getInt() {
            return poll().asInt();
        }

        double getDouble() {
            return poll().asDouble();
        }

        uint16_t getUInt16() {
            return poll().asUInt16();
        }

        template <class T>
        void push(T value) {
            s.push(StackVar(value));
        }

        void swap() {
            StackVar var1 = poll();
            StackVar var2 = poll();
            s.push(var1);
            s.push(var2);
        }

        void pop() {
            s.pop();
        }
    };

} // namespace details