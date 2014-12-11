#ifndef VATIABLE_CONTEXT_H__
#define VATIABLE_CONTEXT_H__

#include <map>
#include <utility>
#include <memory>

class VariableContext;

typedef std::map<std::string, uint16_t> VariableMap;
typedef std::pair<uint16_t, uint16_t> ContextVariableId;
typedef std::shared_ptr<VariableContext> VariableContextPtr;

class VariableContext {
  public:
    VariableContext(uint16_t id = 0): mId(id) {}

    ContextVariableId findContextVariableId(const std::string& name) {
      auto resultIterator = mVariableMap.find(name);
      if (resultIterator != mVariableMap.end()) {
        return ContextVariableId(mId, resultIterator->second);
      } else {
        return ContextVariableId();
      }
    }

    bool variableExistsInContext(const std::string& name) {
      auto resultIterator = mVariableMap.find(name);
      return resultIterator != mVariableMap.end();
    }

    uint16_t newVariable(const std::string name) {
      mHasOverflowed = mVariableMap.size() == UINT16_MAX;
      auto newId = mVariableMap.size();
      return mVariableMap[name] = newId;
    }

    bool hasOverflowed() {
      return mHasOverflowed;
    }

    uint16_t contextId() {return mId;}

    uint16_t localsNumber() {return mVariableMap.size();}

  private:
    uint16_t mId;
    bool mHasOverflowed = false;
    VariableContextPtr mParent;
    VariableMap mVariableMap;
};

#endif
