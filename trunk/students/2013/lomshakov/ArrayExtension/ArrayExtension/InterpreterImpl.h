//
// Created by Vadim Lomshakov on 10/26/13.
// Copyright (c) 2013 spbau. All rights reserved.
//


#ifndef __interpreter_H_
#define __interpreter_H_



#include <map>
#include <set>
#include <cfloat>
#include <stdexcept>
#include <iostream>
#include <AsmJit/AsmJit.h>
#include "ast.h"
#include "mathvm.h"


namespace mathvm {
  // maybe make splitting commands set, and optimized work with stack and access to it

  using namespace AsmJit;

  class SVar {
    union {
      double _doubleValue;
      int64_t _intValue;
      uint32_t _stringRef; // extend for save flag isExternalStr
      void* _ref;
    };
    bool _isRef;
  public:
    SVar() { }
    explicit SVar(void* ptr): _ref(ptr), _isRef(true) {};
    explicit SVar(double val): _doubleValue(val), _isRef(false) {};
    explicit SVar(int64_t val): _intValue(val), _isRef(false) {};
    explicit SVar(uint32_t val): _stringRef(val), _isRef(false) {};

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

    void setStringValue(uint32_t ref) {
      _stringRef = ref;
    }

    uint32_t getStringValue() const {

      return _stringRef;
    }

    void* getRef() const {
      assert(_isRef);
      return _ref;
    }

    bool isRefType() const { return _isRef; }
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

    void pushStringValue(uint32_t ref) {
      push_back(SVar(ref));
    }

    uint32_t popStringValue() {
      uint32_t tmp = back().getStringValue();
      pop_back();
      return tmp;
    }

    void pushRefValue(void* ptr) {
      push_back(SVar(ptr));
    }

    void* popRefValue() {
      void* tmp = back().getRef();
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


    set<sysint_t> buildRootSet() {
      set<sysint_t> roots;
      for (size_t i = 0; i != _mapFrames.size(); ++i) {
        for (size_t j = 0; j != _mapFrames[i].size(); ++j) {
          StackFrame& sframe = _mapFrames[i][j];

          for (size_t l = 0; l != sframe.locals.size(); ++l)
            if (sframe.locals[l].isRefType())
              roots.insert((sysint_t)sframe.locals[l].getRef());

          for (size_t l = 0; l != sframe.stack.size(); ++l)
            if (sframe.stack[l].isRefType())
              roots.insert((sysint_t)sframe.stack[l].getRef());
        }
      }
      return roots;
    }
  protected:

#define GEN_CMD_OMETHOD(c, d, l)     \
    void execute##c();

    FOR_BYTECODES(GEN_CMD_OMETHOD)
#undef GEN_CMD_OMETHOD

  private:

    inline Stack& currentStack() { return currentFrameStack().stack; }
    inline LocalsArray& localsByCtx(uint16_t ctxId) { return _mapFrames[ctxId].back().locals; }
    inline LocalsArray& currentLocals() { return currentFrameStack().locals; }
    inline StackFrame& currentFrameStack() { return _mapFrames[_callStack.top().fid].back(); }

    inline void jump(int16_t offset) {
      assert(ip() + offset >= 0);
      assert((int32_t)ip() + offset - 2 < (int32_t)_currentCode->length());

      _callStack.top().ip += offset - 2 /* because we shifted to two bytes when we was reading the offset of jump instruction */;
    }

    inline uint8_t getNextInsn() {
      uint32_t idx = ip();
      shiftIpOnByte();
      return _currentCode->get(idx);
    }

    inline void shiftIpOnByte() {
      assert(ip() < _currentCode->length());
      ++(_callStack.top().ip);
    }

    inline uint16_t getNextUInt16() {
      uint32_t idx = ip();
      shiftIpOn2Byte();
      return _currentCode->getUInt16(idx);
    }

    inline int16_t getNextInt16() {
      uint32_t idx = ip();
      shiftIpOn2Byte();
      return _currentCode->getInt16(idx);
    }

    inline void shiftIpOn2Byte() {
      assert(ip() + 1 < _currentCode->length());
      _callStack.top().ip += 2;
    }

    inline double getNextDouble() {
      uint32_t idx = ip();
      shiftIpOn8Byte();
      return _currentCode->getDouble(idx);
    }

    inline int64_t getNextInt64() {
      uint32_t idx = ip();
      shiftIpOn8Byte();
      return _currentCode->getInt64(idx);
    }

    inline void shiftIpOn8Byte() {
      assert(ip() + 7 < _currentCode->length());
      _callStack.top().ip += 8;
    }

    inline uint32_t ip() const {
      assert(!_callStack.empty());
      return _callStack.top().ip;
    }

    void bindTopLevelVars(vector<Var *> &vars);
    void unbindTopLevelVars(vector<Var *> &vars);
    void callFunction(uint16_t id);
    void returnFromFunction();
    void setInitialState();

// ================================================================================================================== //
// wrapping constantById and introduce method for make external string

    inline char const * getStringConstant(uint32_t extRef) {
      if ((extRef >> 16) & 1) { // this is external string
        assert(externalPtrs.count((uint16_t) extRef) == 1);
        return (char const*)externalPtrs[(uint16_t) extRef];
      }
      return constantById((uint16_t) extRef).c_str();
    }



    inline uint32_t makeExternalString(char* ptr) {
      assert(externalPtrs.size() < (1 << 16));
      uint16_t sid = (uint16_t) externalPtrs.size();

      assert(externalPtrs.count(sid) == 0); // Ref must be not allocated yet

      externalPtrs[sid] = (int64_t) ptr;
      uint32_t res = (1 << 16) | sid;
      return res;
    }

    map<uint16_t, int64_t> externalPtrs;
  };

}
#endif //__interpreter_H_
