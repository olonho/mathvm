#pragma once

#include <deque>

#include "common.h"

// ================================================================================

namespace mathvm {
  class CompilerPool {
  public:
    void addInfo(AstVar* var) {
      var->setInfo(_vi.alloc());
      info(var)->fv = _fv.alloc();
      info(var)->fv->init(var);
    }

    void addInfo(AstNode* node) {
      node->setInfo(allocNodeInfo());
    }

    FlowVar* allocFlowVar() {
      return _fv.alloc();
    }

    FlowNode* allocFlowNode() {
      return _fn.alloc();
    }

    void alloc(FlowNode** fn) {
      *fn = allocFlowNode();
    }

    void alloc(FlowVar** fv) {
      *fv = allocFlowVar();
    }

  private:
    VarInfo* allocVarInfo() {
      return _vi.alloc();
    }

    NodeInfo* allocNodeInfo() {
      return _ni.alloc();
    }

  private:
    Pool<VarInfo> _vi;
    Pool<NodeInfo> _ni;
    Pool<FlowVar> _fv;
    Pool<FlowNode> _fn;
  };
}
