#pragma once

#include <map>
#include <utility>
#include <memory>

class VariableContext;

typedef std::map<std::string, uint16_t> VariableMap;
typedef std::pair<uint16_t, uint16_t> ContextVariableId;

class VariableContext {
public:
    VariableContext(uint16_t id = 0)
            : id(id) {}

    ContextVariableId findContextVariableId(const std::string &name) {
        auto resultIterator = variableMap.find(name);
        if (resultIterator != variableMap.end()) {
            return ContextVariableId(id, resultIterator->second);
        } else {
            return ContextVariableId();
        }
    }

    bool variableExistsInContext(const std::string &name) {
        auto resultIterator = variableMap.find(name);
        return resultIterator != variableMap.end();
    }

    uint16_t newVariable(const std::string name) {
        overflowed = variableMap.size() == UINT16_MAX;
        auto newId = variableMap.size();
        return variableMap[name] = newId;
    }

    bool hasOverflowed() {
        return overflowed;
    }

    uint16_t contextId() { return id; }

    uint16_t localsNumber() { return variableMap.size(); }

private:
    uint16_t id;
    bool overflowed = false;
    VariableMap variableMap;
};
