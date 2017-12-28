#include "../../../../include/mathvm.h"
#include "include/local_variable_storage.h"

variable_storage::variable_storage(int16_t defaultStringId,
                                   size_t preallocatedMemory,
                                   size_t preallocatedStackFrames) :
        _id_str(defaultStringId) {
    _stack_data.reserve(allocate_memory);
    _stack_frame_counter.reserve(allocate_stack_frame_size);
    _is_present.reserve(allocate_memory);
}

int64_t variable_storage::iload(size_t is_stack_frame, int16_t id) {
    auto objectId = stackFrameWithId(is_stack_frame) + id;
    return _is_present[objectId] ? _stack_data[objectId].value_int : 0;
}

double variable_storage::dload(size_t is_stack_frame, int16_t id) {
    auto objectId = stackFrameWithId(is_stack_frame) + id;
    return _is_present[objectId] ? _stack_data[objectId].value_double : 0.0;
}

int16_t variable_storage::sload(size_t is_stack_frame, int16_t id) {
    auto objectId = stackFrameWithId(is_stack_frame) + id;
    return _is_present[objectId] ? _stack_data[objectId].id : _id_str;
}

size_t variable_storage::currentStackFrame() {
    return _stack_frame_counter.size() - 1;
}

int64_t variable_storage::iload(int16_t id) {
    return iload(this->currentStackFrame(), id);
}

double variable_storage::dload(int16_t id) {
    return dload(this->currentStackFrame(), id);
}

int16_t variable_storage::sload(int16_t id) {
    return sload(this->currentStackFrame(), id);
}

void variable_storage::istore(int16_t id, int64_t value) {
    istore(this->currentStackFrame(), id, value);
}

void variable_storage::dstore(int16_t id, double value) {
    dstore(this->currentStackFrame(), id, value);
}

void variable_storage::sstore(int16_t id, int16_t value) {
    sstore(this->currentStackFrame(), id, value);
}

void variable_storage::istore(size_t is_stack_frame, int16_t id, int64_t value) {
// std::cout << _stack_frame_counter.size() << std::endl;
    auto objectId = stackFrameWithId(is_stack_frame) + id;
    _is_present[objectId] = true;
    _stack_data[objectId].value_int = value;
}

void variable_storage::dstore(size_t is_stack_frame, int16_t id, double value) {
    auto objectId = stackFrameWithId(is_stack_frame) + id;
    _is_present[objectId] = true;
    _stack_data[objectId].value_double = value;
}

void variable_storage::sstore(size_t is_stack_frame, int16_t id, int16_t value) {
    auto objectId = stackFrameWithId(is_stack_frame) + id;
    _is_present[objectId] = true;
    _stack_data[objectId].id = value;
}

void variable_storage::newContext(int32_t localsNumber) {
    auto newSize = _stack_data.size() + localsNumber;
    _stack_frame_counter.push_back(_stack_data.size());
    stacksResize(newSize);
}


void variable_storage::popContext() {
    auto newSize = _stack_frame_counter.back();
    _stack_frame_counter.pop_back();
    stacksResize(newSize);
}

void variable_storage::stacksResize(size_t newSize) {
    _stack_data.resize(newSize);
    _is_present.resize(newSize);
}

size_t variable_storage::stackFrameWithId(size_t is_stack_frame) {
    return _stack_frame_counter[is_stack_frame];
}
