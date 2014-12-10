#ifndef CONTEXT_H__
#define CONTEXT_H__

#include <map>
#include <memory>

#include "data_holder.h"

class Context;

typedef Context* ContextPtr;

class Context {
  public:
    Context(int16_t id = 0): mId(id) {}

    void store(int16_t id, DataHolder holder) {mVariables[id] = holder;}

    DataHolder load(int16_t id, DataHolder defaultValue) {
      if (mVariables.find(id) == mVariables.end()) {
        mVariables[id] = defaultValue;
      }
      return mVariables[id];
    }

    Context newContext() {
      return Context(mId + 1);
    }

    int16_t id() {return mId;}

  private:
    int16_t mId;
    std::map<int16_t, DataHolder> mVariables;
};

#endif
