#ifndef INTERPRETER_UTIL_H
#define INTERPRETER_UTIL_H

#include "mathvm.h"
#include <stack>
#include <unordered_map>
#include <vector>

namespace mathvm {
    namespace utils {

        struct Variable {
            Variable();
            Variable(int64_t value);
            Variable(double value);
            Variable(uint16_t value);
            int64_t intValue();
            double doubleValue();
            uint16_t stringValue();
            VarType& type();

        private:
            union {
                int64_t _intValue;
                double _doubleValue;
                uint16_t _stringValue;
            };

            VarType _type;
        };

        struct Stack {
            int64_t popInt();
            double popDouble();
            uint16_t popUInt16();
            int64_t topInt();
            double topDouble();
            uint16_t topUInt16();
            void pushInt(int64_t value);
            void pushDouble(double value);
            void pushUInt16(uint16_t value);
            VarType tosType();
            void swap();
            Variable pop();

        private:
            Variable top();
            std::stack<Variable> vars;
        };

        struct ContextsVariable {

            void pushScope(uint16_t contextId, uint16_t localsCount);
            void popScope();

            int64_t getInt(uint16_t contextId, uint16_t varId);
            void setInt(uint16_t contextId, uint16_t varId, int64_t value);
            double getDouble(uint16_t contextId, uint16_t varId);
            void setDouble(uint16_t contextId, uint16_t varId, double value);
            uint16_t getUInt16(uint16_t contextId, uint16_t varId);
            void setUInt16(uint16_t contextId, uint16_t varId, uint16_t value);

            // current context
            int64_t getInt(uint16_t varId);
            void setInt(uint16_t varId, int64_t value);
            double getDouble(uint16_t varId);
            void setDouble(uint16_t varId, double value);
            uint16_t getUInt16(uint16_t varId);
            void setUInt16(uint16_t varId, uint16_t value);

            // cached getter setter
            int64_t getCachedInt(uint16_t varId);
            void setCachedInt(uint16_t varId, int64_t value);
            double getCachedDouble(uint16_t varId);
            void setCachedDouble(uint16_t varId, double value);
            uint16_t getCachedUInt16(uint16_t varId);
            void setCachedUInt16(uint16_t varId, uint16_t value);

        private:

            void checkCache(uint16_t varId, VarType type);
            Variable& getVar(uint16_t contextId, uint16_t varId);
            void storeCache();
            void refreshCache();
            Variable _cache[4];
            std::unordered_map<uint16_t, std::stack<std::vector<Variable>>> _variables;
            std::stack<std::pair<uint16_t, uint16_t>> _contextsIdCount;
        };
    }
}

#endif // INTERPRETER_UTIL_H
