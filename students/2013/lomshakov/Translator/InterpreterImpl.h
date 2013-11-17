//
// Created by Vadim Lomshakov on 10/26/13.
// Copyright (c) 2013 spbau. All rights reserved.
//


#ifndef __interpreter_H_
#define __interpreter_H_




#include <cfloat>
#include <stdexcept>
#include <iostream>
#include "ast.h"
#include "mathvm.h"

namespace mathvm {
  // maybe make splitting commands set, and optimized work with stack and access to it

  class SVar {
    union {
      double _doubleValue;
      int64_t _intValue;
      uint16_t _stringRef;
    };
  public:
    SVar() {}
    explicit SVar(double var): _doubleValue(var) {};
    explicit SVar(int64_t val): _intValue(val) {};
    explicit SVar(uint16_t val): _stringRef(val) {};

    void setDoubleValue(double value) {
      _doubleValue = value;
    }

    double getDoubleValue() const {
      return _doubleValue;
    }

    void setIntValue(int64_t value) {
      _intValue = value;
    }

    int64_t getIntValue() const {
      return _intValue;
    }

    void setStringValue(uint16_t ref) {
      _stringRef = ref;
    }

    uint16_t getStringValue() const {

      return _stringRef;
    }
  };

  class Stack: public vector<SVar> {
  public:

    void swap() {
      assert(size() >= 2);
      std::swap((*this)[size()-2], (*this)[size()-1]);
    }

    void push(value_type const &var) {
      push_back(var);
    }

    value_type pop() {
      SVar tmp = back();
      pop_back();
      return tmp;
    }

    void pushDoubleValue(double value) {
      push_back(SVar(value));
    }

    double popDoubleValue() {
      double tmp = back().getDoubleValue();
      pop_back();
      return tmp;
    }

    void pushIntValue(int64_t value) {
      push_back(SVar(value));
    }

    int64_t popIntValue() {
      int64_t tmp = back().getIntValue();
      pop_back();
      return tmp;
    }

    void pushStringValue(uint16_t ref) {
      push_back(SVar(ref));
    }

    uint16_t popStringValue() {
      uint16_t tmp = back().getStringValue();
      pop_back();
      return tmp;
    }
  };

  struct FunctionDescriptor {
    uint32_t ip;
    uint16_t fid;

    static const uint16_t INVALID_FID = 0xffff;

    FunctionDescriptor(uint16_t id = INVALID_FID, uint32_t iip = 0): ip(iip), fid(id) {}
  };

  class CallStack: public vector<FunctionDescriptor> {
  public:
    void push(value_type const& fd) {
      push_back(fd);
    }

    void pop() {
      pop_back();
    }

    value_type& top() {
      return back();
    }

    value_type const & top() const {
      return back();
    }

    value_type& top2() {
      return (*this)[size() - 2];
    }
  };

  typedef vector<SVar> LocalsArray;
  struct StackFrame {
    Stack stack;
    LocalsArray locals;
    StackFrame(size_t countLocals) : locals(countLocals) {}
  };

  class InterpreterImpl : public Code {
    // because range of id function is continuous
    typedef vector<vector<StackFrame> > MapStackFramesByIdFunction;

    Bytecode* _currentCode;
    MapStackFramesByIdFunction _mapFrames;
    // functionId and IP
    CallStack _callStack;

    ostream& out;
  public:
    InterpreterImpl();
    virtual ~InterpreterImpl();

    virtual Status *execute(std::vector<Var *> &vars);

  protected:

#define GEN_CMD_OMETHOD(c, d, l)     \
    void execute##c();

    FOR_BYTECODES(GEN_CMD_OMETHOD)
#undef GEN_CMD_OMETHOD

  private:

    Stack& currentStack() { return currentFrameStack().stack; }
    LocalsArray& localsByCtx(uint16_t ctxId) { return _mapFrames[ctxId].back().locals; }
    LocalsArray& currentLocals() { return currentFrameStack().locals; }
    StackFrame& currentFrameStack() { return _mapFrames[_callStack.top().fid].back(); }

    void jump(int16_t offset) {
      assert(ip() + offset >= 0);
      assert((int32_t)ip() + offset - 2 < (int32_t)_currentCode->length());

      _callStack.top().ip += offset - 2 /* because we shifted to two bytes when we was reading the offset of jump instruction */;
    }

    uint8_t getNextInsn() {
      uint32_t idx = ip();
      shiftIpOnByte();
      return _currentCode->get(idx);
    }

    void shiftIpOnByte() {
      assert(ip() < _currentCode->length());
      ++(_callStack.top().ip);
    }

    uint16_t getNextUInt16() {
      uint32_t idx = ip();
      shiftIpOn2Byte();
      return _currentCode->getUInt16(idx);
    }

    int16_t getNextInt16() {
      uint32_t idx = ip();
      shiftIpOn2Byte();
      return _currentCode->getInt16(idx);
    }

    void shiftIpOn2Byte() {
      assert(ip() + 1 < _currentCode->length());
      _callStack.top().ip += 2;
    }

    double getNextDouble() {
      uint32_t idx = ip();
      shiftIpOn8Byte();
      return _currentCode->getDouble(idx);
    }

    int64_t getNextInt64() {
      uint32_t idx = ip();
      shiftIpOn8Byte();
      return _currentCode->getInt64(idx);
    }

    void shiftIpOn8Byte() {
      assert(ip() + 7 < _currentCode->length());
      _callStack.top().ip += 8;
    }

    uint32_t ip() const {
      assert(!_callStack.empty());
      return _callStack.top().ip;
    }

    void bindTopLevelVars(vector<Var *> &vars);
    void unbindTopLevelVars(vector<Var *> &vars);
    void callFunction(uint16_t id);
    void returnFromFunction();
    void setInitialState();
  };

}
#endif //__interpreter_H_
