#ifndef BYTECODE_INTERPRETER_HPP
#define BYTECODE_INTERPRETER_HPP

#include "mathvm.h"
#include "interpreter_code.hpp"
#include "utils.hpp"

#include <stdint.h>
#include <cassert>
#include <cstddef>

#include <algorithm>

namespace mathvm {

typedef int32_t mem_t;

namespace constants {
  const mem_t MAX_STACK_SIZE = 128*1024*1024;
  const mem_t VAL_SIZE = std::max(sizeof(int64_t), sizeof(double));
}

class StackFrame {
  uint16_t function_;
  uint32_t instruction_;
  mem_t parentFrame_;
  mem_t returnFrame_;

public:
  StackFrame(uint16_t function, uint32_t instruction, mem_t parentFrame, mem_t returnFrame)
    : function_(function),
      instruction_(instruction),
      parentFrame_(parentFrame),
      returnFrame_(returnFrame) {}

  StackFrame(const StackFrame& other) 
    : function_(other.function_),
      instruction_(other.instruction_),
      parentFrame_(other.parentFrame_),
      returnFrame_(other.returnFrame_) {}

  StackFrame& operator=(const StackFrame& other) {
    function_ = other.function_;
    instruction_ = other.instruction_;
    parentFrame_ = other.parentFrame_;
    returnFrame_ = other.returnFrame_;
    return *this;
  }

  uint16_t function() const { return function_; }
  uint32_t instruction() const { return instruction_; }
  mem_t parentFrame() const { return parentFrame_; }
  mem_t returnFrame() const { return returnFrame_; }
};

class BytecodeInterpreter {
  char* stack_;
  InterpreterCodeImpl* code_;
  BytecodeFunction* function_;
  uint32_t instructionPointer_;
  mem_t stackPointer_;
  mem_t stackFramePointer_;

public:
  BytecodeInterpreter(Code* code);
  ~BytecodeInterpreter();
  void execute();

private:
  StackFrame* stackFrame();
  void allocFrame(uint16_t functionId, uint32_t localsNumber, int64_t context);
  void callFunction(uint16_t id);
  void returnFunction();

  template<typename T>
  T* findVar(uint16_t id, uint16_t context) {
    mem_t saveStackFrame = stackFramePointer_;

    while (context > 0) {
      stackFramePointer_ = stackFrame()->parentFrame();
      --context;
    }

    T* t = (T*) (stack_ + stackFramePointer_ + sizeof(StackFrame) + constants::VAL_SIZE * id);
    stackFramePointer_ = saveStackFrame;
    return t;
  }

  enum VarScope { LOCAL, OUTER };

  template<typename T, VarScope scope>
  void loadVar() {
    uint16_t context = 0;
    
    if (scope == OUTER) { 
      context = getTypedAndMoveNext<uint16_t>();
    }

    uint16_t id = getTypedAndMoveNext<uint16_t>(); 
    push(*findVar<T>(id, context));
  }

  template<typename T, VarScope scope>
  void storeVar() {
    uint16_t context = 0;
    
    if (scope == OUTER) { 
      context = getTypedAndMoveNext<uint16_t>();
    }

    uint16_t id = getTypedAndMoveNext<uint16_t>(); 
    *findVar<T>(id, context) = pop<T>();
  }

  void remove() {
    stackPointer_ -= constants::VAL_SIZE;
  }

  template<typename T>
  T* operand() {
    return reinterpret_cast<T*> (stack_ + stackPointer_);
  }

  template<typename T>
  T pop() {
    stackPointer_ -= constants::VAL_SIZE;
    return *operand<T>();
  }

  template<typename T>
  void push(T val) {
    *operand<T>() = val; 
    stackPointer_ += constants::VAL_SIZE;
  }

  template<typename T>
  void load() {
    T val = getTyped<T>();
    instructionPointer_ += sizeof(T);
    push<T>(val);
  }

  void swap() {
    int64_t upper = pop<int64_t>();
    int64_t lower = pop<int64_t>();
    push(upper);
    push(lower);
  }


  Bytecode* bc() {
    return function_->bytecode();
  }

  template<typename T>
  T getTyped() {
    T val = bc()->getTyped<T>(instructionPointer_);
    return val;
  }

  template<typename T>
  T getTypedAndMoveNext() {
    T val = getTyped<T>();
    instructionPointer_ += sizeof(T);
    return val;
  }
};

} // namespace mathvm
#endif