//
// Created by Vadim Lomshakov on 10/26/13.
// Copyright (c) 2013 spbau. All rights reserved.
//


#ifndef __interpreter_H_
#define __interpreter_H_


#include <stack>
#include <map>
#include "mathvm.h"

namespace mathvm {
  class InterpreterCodeImpl: public Code {
    struct StackFrame {
      stack<Var> _stack;
      vector<Var*> locals;
    };
    typedef map<uint16_t , StackFrame*> MapStackFramesByIdFunction;
    typedef stack<pair<uint16_t, uint16_t>> CallStack;   // functionId and IP



    MapStackFramesByIdFunction _stackFrames;
    CallStack _callStack;

  public:
    virtual Status *execute(vector<Var *> &vars);

  private:

  };

}
#endif //__interpreter_H_
