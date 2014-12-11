#ifndef LOCAL_VARIABLE_STORAGE_H__
#define LOCAL_VARIABLE_STORAGE_H__

#include <vector>
#include <utility>

#include "data_holder.h"

class LocalVariableStorage {
  public:
    typedef std::vector<DataHolder> DataStack;
    typedef std::vector<size_t> CounterStack;
    typedef std::vector<bool> PresenceVector;

    static const size_t preallocatedMemorySize = 1024 * 1024;
    static const size_t preallocatedStackFramesSize = 1024 * 1024;

    LocalVariableStorage(int16_t defaultStringId,
        size_t preallocatedMemory = preallocatedMemorySize,
        size_t preallocatedStackFrames = preallocatedStackFramesSize): 
      mDefaultStringId(defaultStringId) {
      mDataStack.reserve(preallocatedMemorySize);
      mStackFrameStack.reserve(preallocatedStackFramesSize);
      mPresenceVector.reserve(preallocatedMemorySize);
    }

    int64_t iload(size_t stackFrameId, int16_t id) {
      auto objectId = stackFrameWithId(stackFrameId) + id;
      return mPresenceVector[objectId] ? mDataStack[objectId].intValue : 0;
    }

    double dload(size_t stackFrameId, int16_t id) {
      auto objectId = stackFrameWithId(stackFrameId) + id;
      return mPresenceVector[objectId] ? mDataStack[objectId].doubleValue : 0.0;
    }

    int16_t sload(size_t stackFrameId, int16_t id) {
      auto objectId = stackFrameWithId(stackFrameId) + id;
      return mPresenceVector[objectId] ? mDataStack[objectId].stringId : mDefaultStringId;
    }

    int64_t iload(int16_t id) {return iload(currentStackFrame(), id);}

    double dload(int16_t id) {return dload(currentStackFrame(), id);}

    int16_t sload(int16_t id) {return sload(currentStackFrame(), id);}

    void istore(int16_t id, int64_t value) {istore(currentStackFrame(), id, value);}

    void dstore(int16_t id, double value) {dstore(currentStackFrame(), id, value);}

    void sstore(int16_t id, int16_t value) {sstore(currentStackFrame(), id, value);}

    void istore(size_t stackFrameId, int16_t id, int64_t value) {
      // std::cout << mStackFrameStack.size() << std::endl;
      auto objectId = stackFrameWithId(stackFrameId) + id;
      mPresenceVector[objectId] = true;
      mDataStack[objectId].intValue = value;
    }

    void dstore(size_t stackFrameId, int16_t id, double value) {
      auto objectId = stackFrameWithId(stackFrameId) + id;
      mPresenceVector[objectId] = true;
      mDataStack[objectId].doubleValue = value;
    }

    void sstore(size_t stackFrameId, int16_t id, int16_t value) {
      auto objectId = stackFrameWithId(stackFrameId) + id;
      mPresenceVector[objectId] = true;
      mDataStack[objectId].stringId = value;
    }

    void newContext(int32_t localsNumber) {
      auto newSize = mDataStack.size() + localsNumber;
      mStackFrameStack.push_back(mDataStack.size());
      stacksResize(newSize);
    }

    void popContext() {
      auto newSize = mStackFrameStack.back();
      mStackFrameStack.pop_back();
      stacksResize(newSize);
    }

  private:
    void stacksResize(size_t newSize) {
      mDataStack.resize(newSize);
      mPresenceVector.resize(newSize);
    }

    size_t currentStackFrame() {
      return mStackFrameStack.size() - 1;
    }

    size_t stackFrameWithId(size_t stackFrameId) {
      return mStackFrameStack[stackFrameId];
    }

    DataStack mDataStack;
    CounterStack mStackFrameStack;
    PresenceVector mPresenceVector;
    const int16_t mDefaultStringId;
};

#endif
