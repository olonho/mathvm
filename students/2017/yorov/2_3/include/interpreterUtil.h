#ifndef INTERPRETER_UTIL_H
#define INTERPRETER_UTIL_H

#include "mathvm.h"
#include <stack>
#include <unordered_map>
#include <vector>
#include <string>

namespace mathvm {
    namespace utils {

        struct Variable {
            Variable();
            Variable(int64_t value);
            Variable(double value);
            Variable(const char* value);
            int64_t intValue();
            double doubleValue();
            const char* stringValue();
            VarType& type();

        private:
            union {
                int64_t _intValue;
                double _doubleValue;
                const char* _stringValue;
            };

            VarType _type;
        };

        struct Stack {
            int64_t popInt();
            double popDouble();
            const char* popString();
            void pushInt(int64_t value);
            void pushDouble(double value);
            void pushString(const char* value);
            VarType tosType();
            void swap();
            Variable pop();

        private:
            std::stack<Variable> vars;
        };

        struct ContextsVariable {

            void pushScope(uint16_t contextId, uint16_t localsCount);
            void popScope();

            int64_t getInt(uint16_t contextId, uint16_t varId);
            void setInt(uint16_t contextId, uint16_t varId, int64_t value);
            double getDouble(uint16_t contextId, uint16_t varId);
            void setDouble(uint16_t contextId, uint16_t varId, double value);
            const char* getString(uint16_t contextId, uint16_t varId);
            void setString(uint16_t contextId, uint16_t varId, const char* value);

            // current context
            int64_t getInt(uint16_t varId);
            void setInt(uint16_t varId, int64_t value);
            double getDouble(uint16_t varId);
            void setDouble(uint16_t varId, double value);
            const char* getString(uint16_t varId);
            void setString(uint16_t varId, const char* value);

            // cached getter setter
            int64_t getCachedInt(uint16_t varId);
            void setCachedInt(uint16_t varId, int64_t value);
            double getCachedDouble(uint16_t varId);
            void setCachedDouble(uint16_t varId, double value);
            const char* getCachedString(uint16_t varId);
            void setCachedString(uint16_t varId, const char* value);

        private:

            void checkCache(uint16_t varId);
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
