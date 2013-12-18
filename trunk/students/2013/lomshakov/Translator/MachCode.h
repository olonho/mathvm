//
// Created by Vadim Lomshakov on 13/12/13.
// Copyright (c) 2013 spbau. All rights reserved.
//


#include "mathvm.h"
#include "ast.h"
#include "VCtx.h"
#include <AsmJit/AsmJit.h>
#include <stdint.h>
#include <set>

#ifndef __MachCode_H_
#define __MachCode_H_


namespace mathvm {

  using namespace AsmJit;


  typedef pair<string, int64_t> VarId;
  typedef pair<int32_t, int32_t> VarAddress;  // offset var,  depth of owner function

  typedef map<VarId, VarAddress> VarAllocation;

  inline
  int64_t getScopeUID(Scope *scope) { return (int64_t) scope; }


  class MachCodeFunc;
  struct VCtx;
  class MachCode : public Code {
    VCtx context;
  public:
    MachCode();
    virtual ~MachCode();

    virtual Status* execute(vector<Var*>& vars);

    MachCodeFunc* functionByName(const string& name);
    MachCodeFunc* functionById(uint16_t id);
    void error(const char* format, ...);

    VCtx& vcontext() { return context; }
  private:
    typedef set<string> StringSet;
    typedef set<string>::iterator StringSetIt;
    // 'data' section
    StringSet _stringConstants;
  public:

    char const * makeOrGetStringConstant(string const& str);
  };

  class MachCodeFunc : public TranslatedFunction {
    void* _code;
    uint32_t _funcDepth;
    uint32_t _nextStackOffset;
  public:
    static const uint16_t VAR_SIZE = 8; // size of any var

    MachCodeFunc(AstFunction* func) :
    TranslatedFunction(func),
    _code(0),
    _funcDepth(0),
    _nextStackOffset(2)
    { }
    virtual ~MachCodeFunc();
    void** machcode() { return &_code; }
    void setCode(void* code) { _code = code; }
    virtual void disassemble(ostream& out) const;

    void setDepth(uint32_t d) { _funcDepth = d; }
    uint32_t getDepth() const { return _funcDepth; }

    void setVStackOffset(uint32_t s) { _nextStackOffset = s; }
    uint32_t getVStackOffset() const { return _nextStackOffset; }

    uint32_t getVStackSize() const { return VAR_SIZE * _nextStackOffset; }
  };


  inline void pprolog(VCtx * vctxPtr, MachCodeFunc* func) {
    VCtx& vctx = *vctxPtr;
    int64_t newVBP = vctx.SP;

    // push VBP
    *((int64_t*)(void*)(vctx.stack + newVBP + VCtx::VBP_OFFSET * MachCodeFunc::VAR_SIZE)) = vctx.BP;

    vctx.BP = vctx.SP;

    vctx.SP += func->getVStackSize();

    assert(func->getDepth() <= MAX_DISPLAY_SIZE);

    // push prev access link
    *((int64_t*)(void*)(vctx.stack + newVBP + VCtx::PREV_LINK_ACCESS_OFFSET * MachCodeFunc::VAR_SIZE)) = vctx.display[func->getDepth()];

    vctx.display[func->getDepth()] = vctx.BP;


    assert(vctx.SP >= 0);
    assert(vctx.BP >= 0);
  }

  inline void pepilog(VCtx * vctxPtr, MachCodeFunc* func) {
    VCtx& vctx = *vctxPtr;

    // pop prev access link
    vctx.display[func->getDepth()] = *((int64_t*)(void*)(vctx.stack + vctx.BP + VCtx::PREV_LINK_ACCESS_OFFSET * MachCodeFunc::VAR_SIZE));

    // pop VBP
    vctx.BP = *((int64_t*)(void*)(vctx.stack + vctx.BP + VCtx::VBP_OFFSET * MachCodeFunc::VAR_SIZE));

    vctx.SP -= func->getVStackSize();

    assert(vctx.SP >= 0);
    assert(vctx.BP >= 0);
  }


  inline void* getVarAddress(int32_t offsetOnStk, int32_t idxDisplay, VCtx * vctxPtr) {
    VCtx& vctx = *vctxPtr;

    int64_t vbpOffset = vctx.display[idxDisplay];
    int64_t varOffset = vbpOffset + offsetOnStk * MachCodeFunc::VAR_SIZE;

    assert(varOffset < MAX_STACK_SIZE - MachCodeFunc::VAR_SIZE);
    return (void*)(vctx.stack + varOffset);
  }


}

#endif //__MachCode_H_
