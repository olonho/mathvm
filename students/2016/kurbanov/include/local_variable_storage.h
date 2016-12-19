#pragma once

#include <vector>
#include <utility>
#include <cstddef>

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
                         size_t preallocatedStackFrames = preallocatedStackFramesSize)
            :
            defaultStringId(defaultStringId) {
        dataStack.reserve(preallocatedMemorySize);
        stackFrameStack.reserve(preallocatedStackFramesSize);
        presenceVector.reserve(preallocatedMemorySize);
    }

    int64_t iload(size_t stackFrameId, int16_t id) {
        auto objectId = stackFrameWithId(stackFrameId) + id;
        return presenceVector[objectId] ? dataStack[objectId].intValue : 0;
    }

    double dload(size_t stackFrameId, int16_t id) {
        auto objectId = stackFrameWithId(stackFrameId) + id;
        return presenceVector[objectId] ? dataStack[objectId].doubleValue : 0.0;
    }

    int16_t sload(size_t stackFrameId, int16_t id) {
        auto objectId = stackFrameWithId(stackFrameId) + id;
        return presenceVector[objectId] ? dataStack[objectId].stringId : defaultStringId;
    }

    int64_t iload(int16_t id) { return iload(currentStackFrame(), id); }

    double dload(int16_t id) { return dload(currentStackFrame(), id); }

    int16_t sload(int16_t id) { return sload(currentStackFrame(), id); }

    void istore(int16_t id, int64_t value) { istore(currentStackFrame(), id, value); }

    void dstore(int16_t id, double value) { dstore(currentStackFrame(), id, value); }

    void sstore(int16_t id, int16_t value) { sstore(currentStackFrame(), id, value); }

    void istore(size_t stackFrameId, int16_t id, int64_t value) {
        // std::cout << stackFrameStack.size() << std::endl;
        auto objectId = stackFrameWithId(stackFrameId) + id;
        presenceVector[objectId] = true;
        dataStack[objectId].intValue = value;
    }

    void dstore(size_t stackFrameId, int16_t id, double value) {
        auto objectId = stackFrameWithId(stackFrameId) + id;
        presenceVector[objectId] = true;
        dataStack[objectId].doubleValue = value;
    }

    void sstore(size_t stackFrameId, int16_t id, int16_t value) {
        auto objectId = stackFrameWithId(stackFrameId) + id;
        presenceVector[objectId] = true;
        dataStack[objectId].stringId = value;
    }

    void newContext(int32_t localsNumber) {
        auto newSize = dataStack.size() + localsNumber;
        stackFrameStack.push_back(dataStack.size());
        stacksResize(newSize);
    }

    void popContext() {
        auto newSize = stackFrameStack.back();
        stackFrameStack.pop_back();
        stacksResize(newSize);
    }

private:
    void stacksResize(size_t newSize) {
        dataStack.resize(newSize);
        presenceVector.resize(newSize);
    }

    size_t currentStackFrame() {
        return stackFrameStack.size() - 1;
    }

    size_t stackFrameWithId(size_t stackFrameId) {
        return stackFrameStack[stackFrameId];
    }

    DataStack dataStack;
    CounterStack stackFrameStack;
    PresenceVector presenceVector;
    const int16_t defaultStringId;
};

