#ifndef VIRTUAL_MACHINES_MYINTERPRETERCODEIMPL_H
#define VIRTUAL_MACHINES_MYINTERPRETERCODEIMPL_H

#include "mathvm.h"

namespace mathvm {
    class InterpreterCodeImpl : public Code {
    public:
        Status *execute(vector<Var *> &vars);

    };

    class Value {
        union {
            double dVal;
            int64_t iVal;
            const char *sVal;
        };
    public:
        Value() {}

        Value(const char *stringValue) : sVal(stringValue) {}

        Value(double doubleValue) : dVal(doubleValue) {}

        Value(int64_t intValue) : iVal(intValue) {}

        void setDoubleValue(double value) {
            dVal = value;
        }

        double getDoubleValue() const {
            return dVal;
        }

        void setIntValue(int64_t value) {
            iVal = value;
        }

        int64_t getIntValue() const {
            return iVal;
        }

        void setStringValue(const char *value) {
            sVal = value;
        }

        const char *getStringValue() const {
            return sVal;
        }
    };

    class Context {
    private:
        vector<Value> localVars;

    public:
        Context(const uint32_t &localsNumbers) : localVars(localsNumbers) {};

        Value &getById(uint16_t idx) {
            Value &value = localVars[idx];
            return value;
        }

    };


    class Environment {
    private:
        vector<vector<Context>> funcContexts;

    public:
        Environment(uint32_t funcNumber) : funcContexts(funcNumber) {}

        double getDouble(uint16_t scopeid, uint16_t localid) {
            return funcContexts[scopeid].back().getById(localid).getDoubleValue();
        }

        int64_t getInt(uint16_t scopeid, uint16_t localid) {
            return funcContexts[scopeid].back().getById(localid).getIntValue();
        }

        const char *getString(uint16_t scopeid, uint16_t localid) {
            return funcContexts[scopeid].back().getById(localid).getStringValue();
        }

        void setDouble(uint16_t scopeid, uint16_t localid, const double &value) {
            funcContexts[scopeid].back().getById(localid).setDoubleValue(value);
        }

        void setInt(uint16_t scopeid, uint16_t localid, const int64_t &value) {
            funcContexts[scopeid].back().getById(localid).setIntValue(value);
        }

        void setString(uint16_t scopeid, uint16_t localid, const char *value) {
            funcContexts[scopeid].back().getById(localid).setStringValue(value);
        }

        void emplace(uint16_t scopeid, uint32_t localsNumbers) {
            funcContexts[scopeid].emplace_back(localsNumbers);
        }

        void pop(uint16_t scopeid) {
            funcContexts[scopeid].pop_back();
        }
    };

}


#endif //VIRTUAL_MACHINES_MYINTERPRETERCODEIMPL_H
