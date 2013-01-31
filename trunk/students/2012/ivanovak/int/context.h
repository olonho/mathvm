#ifndef INT_CONTEXT_H_
#define INT_CONTEXT_H_

#include <stdint.h>
#include "mathvm.h"

union StackEntry {
  int64_t i64;
  double d;
  int16_t i16;
  StackEntry() {}
  StackEntry(const StackEntry& other) {d = other.d;}
  StackEntry(double a) : d(a) {}
  StackEntry(int64_t a) : i64(a) {}
  StackEntry(int16_t a) : i16(a) {}

  operator int16_t() {return i16;}
  operator int64_t() {return i64;}
  operator double() {return d;}
};

class Context {
public:
  Context() : curSize(0) {}
  
  void addFun(mathvm::BytecodeFunction *f) {
    resizeOnDemand(f->id());
    bc_functions[f->id()] = f;
    localsNumber[f->id()] = f->localsNumber();
  }

  template <class T>
  T getVar(int16_t fun, int16_t var) {
    return static_cast<T>(fun_variables[fun][fun_variables[fun].size() - var]);
  }

  void setVar(int16_t fun, int16_t var, StackEntry value) {
    fun_variables[fun][fun_variables[fun].size() - var] = value;
  }

  void generateStackFrame(int16_t index) {
    int oldSize = fun_variables[index].size();
    fun_variables[index].resize(oldSize + localsNumber[index]);
  }
  
  mathvm::BytecodeFunction* getFunById(int16_t funIndex) {
    return bc_functions[funIndex];
  }

  void dropStackFrame(int16_t index) {
    int oldSize = fun_variables[index].size();
    fun_variables[index].resize(oldSize - localsNumber[index]);
  }
private:
  std::vector<std::vector<StackEntry> > fun_variables;
  std::vector<int> localsNumber;
  std::vector<mathvm::BytecodeFunction*> bc_functions;
  uint16_t curSize;
  
  void resizeOnDemand(uint16_t i) {
    if (i >= curSize) {
      curSize = i + 1;
      fun_variables.resize(curSize);
      localsNumber.resize(curSize);
      bc_functions.resize(curSize);
    }  
  }
};


#endif /* INT_CONTEXT_H_ */
