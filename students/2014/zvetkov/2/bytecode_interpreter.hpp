#ifndef BYTECODE_INTERPRETER_HPP
#define BYTECODE_INTERPRETER_HPP

#include "mathvm.h"
#include "interpreter_code.hpp"
#include "utils.hpp"

#include <stdint.h>
#include <cassert>
#include <cstddef>

namespace mathvm {

namespace constants {
  const int64_t MAX_STACK_SIZE = 128000000;
}

class BytecodeInterpreter {
  static const int64_t VAL_SIZE = 8;

  struct StackFrame {
    uint16_t id;
    uint32_t ins;
    int64_t sp;
    int64_t sf;
  };

  InterpreterCodeImpl* code_;
  BytecodeFunction* function_;
  char* stack_;
  uint32_t ins_;
  int64_t sp_; // current stack pointer
  int64_t sf_; // current stack frame

public:
  BytecodeInterpreter(Code* code): ins_(0), sp_(0), sf_(0) {
    stack_ = new char[constants::MAX_STACK_SIZE];
    code_ = dynamic_cast<InterpreterCodeImpl*>(code);
    assert(code_ != NULL);
    function_ = code_->functionById(0);
    callFunction(0);
  }

  ~BytecodeInterpreter() {
    delete [] stack_;
  }

  void execute();

private:
  void callFunction(uint16_t id) {
    StackFrame* frame = (StackFrame*) (stack_ + sp_);
    frame->id = function_->id();
    frame->ins = ins_;
    frame->sp = sp_;
    frame->sf = sf_;

    ins_ = 0;
    sf_ = sp_;

    function_ = code_->functionById(id);
    sp_ += sizeof(StackFrame) + VAL_SIZE * function_->localsNumber();
  } 

  void returnFunction() {
    uint64_t returnValue = pop<uint64_t>();
    
    StackFrame* frame = (StackFrame*) (stack_ + sf_);
    uint16_t id = frame->id;
    ins_ = frame->ins;
    sp_ = frame->sp;
    sf_ = frame->sf;

    function_ = code_->functionById(id);
    push(returnValue);
  }

  template<typename T>
  T* findVar(uint16_t id, uint16_t context) {
    int64_t sfSave = sf_;

    while (context > 0) {
      StackFrame* frame = (StackFrame*) (stack_ + sf_);
      sf_ = frame->sf;
      --context;
    }

    T* t = (T*) (stack_ + sf_ + sizeof(StackFrame) + VAL_SIZE * id);
    sf_ = sfSave;
    return t;
  }

  template<typename T>
  void loadVar(uint16_t id, uint16_t context) {
    push(*findVar<T>(id, context));
  }

  template<typename T>
  void storeVar(uint16_t id, uint16_t context, T val) {
    *findVar<T>(id, context) = val;
  }

  Bytecode* bc() {
    return function_->bytecode();
  }

  void remove() {
    sp_ -= VAL_SIZE;
  }

  template<typename T>
  T pop() {
    sp_ -= VAL_SIZE;
    return *((T*) (stack_ + sp_));
  }

  template<typename T>
  void push(T val) {
    *((T*) (stack_ + sp_)) = val; 
    sp_ += VAL_SIZE;
  }

  template<typename T>
  void load() {
    T val = readFromBc<T>();
    ins_ += sizeof(T);
    push<T>(val);
  }

  template<typename T>
  T readFromBc() {
    T val = bc()->getTyped<T>(ins_);
    return val;
  }

  template<typename T>
  T readFromBcAndShift() {
    T val = bc()->getTyped<T>(ins_);
    ins_ += sizeof(T);
    return val;
  }

  void swap() {
    int64_t upper = pop<int64_t>();
    int64_t lower = pop<int64_t>();
    push(upper);
    push(lower);
  }
};

} // namespace mathvm
#endif