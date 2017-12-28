#ifndef LOCAL_VARIABLE_STORAGE_H__
#define LOCAL_VARIABLE_STORAGE_H__

#include <vector>
#include <utility>

#include "data.h"

class variable_storage {
public:
    typedef std::vector<data> stack_data;
    typedef std::vector<size_t> stack_counter;
    typedef std::vector<bool> is_present_vector;

    static const size_t allocate_memory = 1024 * 1024;
    static const size_t allocate_stack_frame_size = 1024 * 1024;

    variable_storage(int16_t defaultStringId,
                     size_t preallocatedMemory = allocate_memory,
                     size_t preallocatedStackFrames = allocate_stack_frame_size);

    int64_t iload(size_t is_stack_frame, int16_t id);

    double dload(size_t is_stack_frame, int16_t id);

    int16_t sload(size_t is_stack_frame, int16_t id);

    int64_t iload(int16_t id);

    double dload(int16_t id);

    int16_t sload(int16_t id);

    void istore(int16_t id, int64_t value);

    void dstore(int16_t id, double value);

    void sstore(int16_t id, int16_t value);

    void istore(size_t is_stack_frame, int16_t id, int64_t value);

    void dstore(size_t is_stack_frame, int16_t id, double value);

    void sstore(size_t is_stack_frame, int16_t id, int16_t value);

    void newContext(int32_t localsNumber);

    void popContext();

private:
    void stacksResize(size_t newSize);

    size_t currentStackFrame();

    size_t stackFrameWithId(size_t is_stack_frame);


    stack_data _stack_data;
    stack_counter _stack_frame_counter;
    is_present_vector _is_present;
    const int16_t _id_str;
};

#endif
