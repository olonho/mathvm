#ifndef CONTEXT_H__
#define CONTEXT_H__

#include <map>
#include <memory>

#include "data_holder.h"

class Context;

typedef std::shared_ptr<Context> ContextPtr;

class Context {
  public:
    Context(ContextPtr parentContext = ContextPtr()):
      mId(parentContext ? parentContext->id() + 1 : 0) {}

    void store(int16_t id, DataHolder holder) {mVariables[id] = holder;}
    DataHolder load(int16_t id) {return mVariables[id];}

    int16_t id() {return mId;}

  private:
    int16_t mId;
    std::map<int16_t, DataHolder> mVariables;
};

#endif
