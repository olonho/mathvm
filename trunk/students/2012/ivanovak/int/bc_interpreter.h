#ifndef INT_BC_INTERPRETER_H_
#define INT_BC_INTERPRETER_H_

#include <vector>
#include <stack>
#include <iostream>

#include "context.h"
#include "mathvm.h"

/**
  Interpreter witch not corresponds to the given codes standart
  for DUMP and STOP function.
*/
class BytecodeInterpretator : public mathvm::Code {
public:
  BytecodeInterpretator();
  //BytecodeInterpretator(mathvm::Code&);
  mathvm::Status* execute(std::vector<mathvm::Var*> &vars);
  /* Function call */
  void callById(int id);
  //void setCode(mathvm::Code& acode) { code = acode; };
private:
  Context *context;
  static const int ONE_BYTE_RANGE = 256; 
  /* Runtime stack */
  std::stack<StackEntry> rtstack;
  /* Push shortcut */
  template <class T>
  void push(T s) {
    rtstack.push(static_cast<StackEntry>(s));
  }
  /* Pop shortcut */
  template <class T>
  T pop() {
    T res = static_cast<T>(rtstack.top());
    rtstack.pop();
    return res;
  }
  /* Top shortcut */
  template <class T>
  T top() {
    T res = static_cast<T>(rtstack.top());
    return res;
  }
  mathvm::BytecodeFunction *funcs;
  std::vector<int> lengths;
  void initLengths() {
    lengths.resize(ONE_BYTE_RANGE);
#define UPDATE_LENGTH(b, d, l) lengths[(mathvm::BC_##b)] = l;
    FOR_BYTECODES(UPDATE_LENGTH)
#undef UPDATE_LENGTH
  }
};

#endif /* INT_BC_INTERPRETER_H_ */
