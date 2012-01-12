#pragma once

#include <deque>

#include "common.h"

// ================================================================================

namespace mathvm {
  class CompilerPool {
  public:
    enum PredefConst {
      IntZero,
      StringNull,
      PredefConstMax
    };

    CompilerPool() {
      FlowVar* fv;

      fv = &_predefs[IntZero];
      fv->_type = VT_INT;
      fv->_stor = FlowVar::STOR_CONST;
      fv->_const.intConst = 0;

      fv = &_predefs[StringNull];
      fv->_type = VT_STRING;
      fv->_stor = FlowVar::STOR_CONST;
      fv->_const.stringConst = NULL;
    }

    FlowVar* getPredefConst(PredefConst c) {
      return _predefs + c;
    }

    void addInfo(AstVar* var) {
      var->setInfo(_vi.alloc());
      info(var)->fv = _fv.alloc();
      info(var)->fv->init(var);
    }

    void addInfo(AstNode* node) {
      node->setInfo(allocNodeInfo());
      info(node)->callList = NULL;
    }

    FlowVar* allocFlowVar() {
      return _fv.alloc();
    }

    FlowNode* allocFlowNode() {
      FlowNode* node = _fn.alloc();

      node->refList = node->refNode = NULL;
      node->prev = node->next = NULL;
      node->offset = INVALID_OFFSET;
      
      return node;
    }

    void alloc(FlowNode** fn) {
      *fn = allocFlowNode();
    }

    void alloc(FlowVar** fv) {
      *fv = allocFlowVar();
    }

    void alloc(VarInfo** vi) {
      *vi = allocVarInfo();
    }

  private:
    VarInfo* allocVarInfo() {
      return _vi.alloc();
    }

    NodeInfo* allocNodeInfo() {
      return _ni.alloc();
    }

  private:
    FlowVar _predefs[PredefConstMax];

    Pool<VarInfo> _vi;
    Pool<NodeInfo> _ni;
    Pool<FlowVar> _fv;
    Pool<FlowNode> _fn;
  };
}
