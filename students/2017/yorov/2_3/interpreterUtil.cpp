#include "interpreterUtil.h"
#include <string>

namespace mathvm {
    namespace utils {

        Variable::Variable()
            : _type(VT_INVALID)
        {}

        Variable::Variable(int64_t value)
            : _intValue(value)
            , _type(VT_INT)
        {}

        Variable::Variable(double value)
            : _doubleValue(value)
            , _type(VT_DOUBLE)
        {}

        Variable::Variable(uint16_t value)
            : _stringValue(value)
            , _type(VT_STRING)
        {}

        int64_t Variable::intValue() {
            return _intValue;
        }

        double Variable::doubleValue() {
            return _doubleValue;
        }

        uint16_t Variable::stringValue() {
            return _stringValue;
        }

        VarType& Variable::type() {
            return _type;
        }

        Variable Stack::pop() {
            assert(!vars.empty() && "values's stack is empty");
            Variable var = vars.top();
            vars.pop();
            return var;
        }

        int64_t Stack::popInt() {
            return pop().intValue();
        }

        double Stack::popDouble() {
            return pop().doubleValue();
        }

        uint16_t Stack::popUInt16() {
            return pop().stringValue();
        }

        void Stack::pushInt(int64_t value) {
            vars.push(Variable(value));
        }

        void Stack::pushDouble(double value) {
            vars.push(Variable(value));
        }

        void Stack::pushUInt16(uint16_t value) {
            vars.push(Variable(value));
        }

        VarType Stack::tosType() {
            assert(!vars.empty() && "stack of types is empty");
            return vars.top().type();
        }

        void Stack::swap() {
            Variable upper = pop();
            Variable lower = pop();
            vars.push(upper);
            vars.push(lower);
        }
        // end stack

        void ContextsVariable::pushScope(uint16_t contextId, uint16_t localsCount) {
            if (!_contextsIdCount.empty()) {
                storeCache();
            }
            _contextsIdCount.push(std::make_pair(contextId, localsCount));
            if (_variables.find(contextId) == _variables.end()) {
                _variables.emplace(contextId, std::stack<std::vector<Variable>>());
            }
            _variables[contextId].push(std::vector<Variable>(localsCount));
            refreshCache();
        }

        void ContextsVariable::popScope() {
            assert(!_contextsIdCount.empty());
            _variables[_contextsIdCount.top().first].pop();
            _contextsIdCount.pop();
            if (!_contextsIdCount.empty()) {
                refreshCache();
            }
        }

        void ContextsVariable::checkCache(uint16_t varId) {
            assert(varId < 4);
        }

        int64_t ContextsVariable::getCachedInt(uint16_t varId) {
            checkCache(varId);
            return _cache[varId].intValue();
        }

        void ContextsVariable::setCachedInt(uint16_t varId, int64_t value) {
            checkCache(varId);
            _cache[varId] = Variable(value);
        }

        double ContextsVariable::getCachedDouble(uint16_t varId) {
            checkCache(varId);
            return _cache[varId].doubleValue();
        }

        void ContextsVariable::setCachedDouble(uint16_t varId, double value) {
            checkCache(varId);
            _cache[varId] = Variable(value);
        }

        uint16_t ContextsVariable::getCachedUInt16(uint16_t varId) {
            checkCache(varId);
            return _cache[varId].stringValue();
        }

        void ContextsVariable::setCachedUInt16(uint16_t varId, uint16_t value) {
            assert(varId < 4);
            _cache[varId] = Variable(value);
        }

        int64_t ContextsVariable::getInt(uint16_t contextId, uint16_t varId) {
            if (contextId == _contextsIdCount.top().first && varId < 4) {
                return getCachedInt(varId);
            }

            Variable& value = getVar(contextId, varId);
            return value.intValue();
        }

        void ContextsVariable::setInt(uint16_t contextId, uint16_t varId, int64_t value) {
            getVar(contextId, varId) = Variable(value);
        }

        double ContextsVariable::getDouble(uint16_t contextId, uint16_t varId) {
            if (contextId == _contextsIdCount.top().first && varId < 4) {
                return getCachedDouble(varId);
            }

            Variable& value = getVar(contextId, varId);
            return value.doubleValue();
        }

        void ContextsVariable::setDouble(uint16_t contextId, uint16_t varId, double value) {
            getVar(contextId, varId) = Variable(value);
        }

        uint16_t ContextsVariable::getUInt16(uint16_t contextId, uint16_t varId) {
            if (contextId == _contextsIdCount.top().first && varId < 4) {
                return getCachedUInt16(varId);
            }

            Variable& value = getVar(contextId, varId);
            return value.stringValue();
        }

        void ContextsVariable::setUInt16(uint16_t contextId, uint16_t varId, uint16_t value) {
            getVar(contextId, varId) = Variable(value);
        }

        // get set for current context
        int64_t ContextsVariable::getInt(uint16_t varId) {
            return getInt(_contextsIdCount.top().first, varId);
        }

        void ContextsVariable::setInt(uint16_t varId, int64_t value) {
            setInt(_contextsIdCount.top().first, varId, value);
        }

        double ContextsVariable::getDouble(uint16_t varId) {
            return getDouble(_contextsIdCount.top().first, varId);
        }
        void ContextsVariable::setDouble(uint16_t varId, double value) {
            setDouble(_contextsIdCount.top().first, varId, value);
        }

        uint16_t ContextsVariable::getUInt16(uint16_t varId) {
            return getUInt16(_contextsIdCount.top().first, varId);
        }

        void ContextsVariable::setUInt16(uint16_t varId, uint16_t value) {
            setUInt16(_contextsIdCount.top().first, varId, value);
        }

        Variable& ContextsVariable::getVar(uint16_t contextId, uint16_t varId) {
            assert(_variables.find(contextId) != _variables.end());
            return _variables[contextId].top()[varId];
        }

        void ContextsVariable::storeCache() {
            auto ctxIdCount = _contextsIdCount.top();
            size_t cacheCount = ctxIdCount.second < 4 ? ctxIdCount.second : 4;
            for (size_t i = 0; i < cacheCount; ++i) {
                switch (_cache[i].type()) {
                    case VT_INT:
                        setInt(ctxIdCount.first, i, _cache[i].intValue());
                        break;

                    case VT_DOUBLE:
                        setDouble(ctxIdCount.first, i, _cache[i].doubleValue());
                        break;

                    case VT_STRING:
                        setUInt16(ctxIdCount.first, i, _cache[i].stringValue());
                        break;

                    default: break;
                }
            }
        }

        void ContextsVariable::refreshCache() {
            auto ctxIdCount = _contextsIdCount.top();
            size_t cacheCount = ctxIdCount.second < 4 ? ctxIdCount.second : 4;
            for (size_t i = 0; i < cacheCount; ++i) {
                _cache[i] = getVar(ctxIdCount.first, i);
            }
        }

    }
}
