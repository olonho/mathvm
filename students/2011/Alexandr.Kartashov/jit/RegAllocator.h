#pragma once

#include "common.h"

#include "x86.h"

// ================================================================================

namespace mathvm {
  #define RESERVED_INT_REGS 3

  class RegAllocator : private AstVisitor {
  public:
    RegAllocator(AstFunction* root) {
      root->node()->visitChildren(this);
    }

  private:
    VISIT(IntLiteralNode) {
      FlowVar* dst = info(node)->fn->u.op.u.copy.to;
      dst->_stor = FlowVar::STOR_REGISTER;
      dst->_storIdx = _curReg;
    }

    VISIT(DoubleLiteralNode) {
      FlowVar* dst = info(node)->fn->u.op.u.copy.to;
      dst->_stor = FlowVar::STOR_REGISTER;
      dst->_storIdx = _curReg;
    }

    VISIT(BinaryOpNode) {
      if (!isLogic(node->kind())) {
        info(node)->fn->u.op.result->_stor = FlowVar::STOR_REGISTER;
        info(node)->fn->u.op.result->_storIdx = _curReg;

        node->left()->visit(this);
        
        _curReg++;
        node->right()->visit(this);
        
        _curReg--;
      } else {
        node->visitChildren(this);
      }
    }

    VISIT(UnaryOpNode) {
      if (!isLogic(node->kind())) {
        info(node)->fn->u.op.result->_stor = FlowVar::STOR_REGISTER;
        info(node)->fn->u.op.result->_storIdx = _curReg;
      }
        
      node->visitChildren(this);
    }

    VISIT(StoreNode) {
      allocInSubtree(node->value());        
    }

    VISIT(LoadNode) {
      info(node)->fn->u.op.result->_stor = FlowVar::STOR_REGISTER;
      info(node)->fn->u.op.result->_storIdx = _curReg;
    }

    VISIT(ForNode) {
      node->visitChildren(this);
    }

    VISIT(WhileNode) {
      node->visitChildren(this);
    }
    
    VISIT(IfNode) {
      node->visitChildren(this);
    }
          
    VISIT(BlockNode) {
      for (uint32_t i = 0; i < node->nodes(); ++i) {
        allocInSubtree(node->nodeAt(i));
      }
    }

    VISIT(PrintNode) {
      for (uint32_t i = 0; i < node->operands(); ++i) {
        allocInSubtree(node->operandAt(i));
      }
    }

    VISIT(CallNode) {
      for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        allocInSubtree(node->parameterAt(i));
      }
    }

    VISIT(ReturnNode) {
      if (node->returnExpr()) {
        allocInSubtree(node->returnExpr());
      }
    }

    void allocInSubtree(AstNode* node) {
      switch (info(node)->type) {
      case VAL_INT:
        if (info(node)->depth > INT_REGS - RESERVED_INT_REGS) {
          ABORT("Unable to allocate registers yet");
        }

        break;

      case VAL_DOUBLE:
        if (info(node)->depth > DOUBLE_REGS) {
          ABORT("Unable to allocate registers yet");
        }
        break;

      default:
        break;
      }

      _curReg = 0;
      node->visit(this);
    }

  private:
    unsigned int _curReg;
  };
}
