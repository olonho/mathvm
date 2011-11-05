#pragma once

#include <deque>
#include <map>
#include <set>
#include <iterator>

#include "compiler.h"

#include "RegAllocator.h"

// ================================================================================

namespace mathvm {
  class Flow : private AstVisitor {

  public:
    Flow(AstFunction* root, CompilerPool* pool) {
      Flattener f(root, pool);
      RegAllocator ra(root);

        //squash(f.first());

      _pool = pool;
      _first = f.first();
      info(root->node())->fn = _first;
      //root->node()->visit(this);
    }

    FlowNode* first() {
      return _first;
    }

  private:
    template<typename T>
    class Eq {
      void eq(T v1, T v2) {
        _eqm.insert(std::make_pair(v1, v2));
      }

      bool isEq(T v1, T v2) const {
        return _eqm.find(std::make_pair(v1, v2)) != _eqm.end();
      }

    private:
      std::set<std::pair<T, T> > _eqm;
    };
    
    /*
    void squash(FlowNode* node) {
      FlowNode* n;
      //Eq m;

      for (n = node; n = n->next; ++n) {
        //if (
      }
      }
    */

    // --------------------------------------------------------------------------------
    
    class Flattener : private AstVisitor {
      typedef std::deque<FlowNode*> FlowOrder;

    public:
      Flattener(AstFunction* root, CompilerPool* pool) {
        _pool = pool;
        _ncur = NULL;

        root->node()->body()->visit(this);

        _ncur->next = NULL;        
      }

      FlowNode* first() {
        return _first;
      }

    private:
      void attach(FlowNode* node) {
        if (_ncur) {
          _ncur->next = node;
        } else {
          _first = node;
        }

        _ncur = node;
      }

      VISIT(BinaryOpNode) {
        FlowNode* fn;

        node->visitChildren(this);

        _pool->alloc(&fn);
        info(node)->fn = fn;
        switch (node->kind()) {
        case tADD:
          fn->type = FlowNode::ADD;
          break;

        case tSUB:
          fn->type = FlowNode::SUB;
          break;

        case tMUL:
          fn->type = FlowNode::MUL;
          break;

        case tDIV:
          fn->type = FlowNode::DIV;
          break;

        case tAND:
          fn->type = FlowNode::AND;
          break;

        case tOR:
          fn->type = FlowNode::OR;
          break;

        case tLT:
          fn->type = FlowNode::LT;
          break;

        case tLE:
          fn->type = FlowNode::LE;
          break;

        case tGT:
          fn->type = FlowNode::GT;
          break;

        case tGE:
          fn->type = FlowNode::GE;
          break;

        case tEQ:
          fn->type = FlowNode::EQ;
          break;

        case tNEQ:
          fn->type = FlowNode::NEQ;
          break;

        default:
          ABORT("Not supported");
        }

        fn->u.op.u.bin.op1 = info(node->left())->fn->u.op.result;
        fn->u.op.u.bin.op2 = info(node->right())->fn->u.op.result;

        /*
        if (isArith(node->kind()) && node->kind() != tDIV) {
          fn->u.op.result = fn->u.op.u.bin.op1;
        } else {
        */
        FlowVar* res;
        _pool->alloc(&res);
        res->_type = (VarType)info(node)->type;
        info(node)->fn->u.op.result = res;
          //}
        
        attach(fn);
      }

      VISIT(UnaryOpNode) {
        node->visitChildren(this);

        FlowNode* fn = _pool->allocFlowNode();
        info(node)->fn = fn;
        switch (node->kind()) {
        case tNOT:
          fn->type = FlowNode::NOT;
          break;

        case tSUB:
          fn->type = FlowNode::NEG;
          break;
          
        default:
          ABORT("Not supported");
        }

        FlowVar* res = _pool->allocFlowVar();
        res->_type = info(node->operand())->fn->u.op.result->_type;

        fn->u.op.u.un.op = info(node->operand())->fn->u.op.result;
        fn->u.op.result = res;
        attach(fn);
      }

      VISIT(LoadNode) {
        FlowNode* fn;
        FlowVar* res;

        node->visitChildren(this);

        _pool->alloc(&fn);
        _pool->alloc(&res);
        res->_type = node->var()->type();

        fn->type = FlowNode::COPY;
        fn->u.op.u.copy.from = info(node->var())->fv;
        fn->u.op.result = fn->u.op.u.copy.to = res;
        
        info(node)->fn = fn;
        attach(fn);
      }
   
      VISIT(StoreNode) {
        node->visitChildren(this);

        FlowNode* fn;
        _pool->alloc(&fn);
        fn->type = FlowNode::COPY;
        fn->u.op.u.copy.from = info(node->value())->fn->u.op.result;
        fn->u.op.u.copy.to = info(node->var())->fv;
        fn->u.op.result = NULL;
                
        info(node)->fn = fn;
        attach(fn);
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
        node->visitChildren(this);
      }

      VISIT(PrintNode) {
        FlowNode* fn;

        size_t args = 0;
        for (uint32_t i = 0; i < node->operands(); ++i) {
          AstNode* op = node->operandAt(i);

          if (!op->isStringLiteralNode()) {
            op->visit(this);
            
            _pool->alloc(&fn);
            fn->type = FlowNode::PUSH;
            fn->u.op.u.un.op = info(op)->fn->u.op.result;
            attach(fn);
            
            args++;
          }
        }

        _pool->alloc(&fn);
        fn->type = FlowNode::PRINT;
        fn->u.print.ref = node;
        fn->u.print.args = args;
        attach(fn);
      }

      VISIT(CallNode) {
        node->visitChildren(this);

        FlowNode* fn = _pool->allocFlowNode();
        fn->type = FlowNode::CALL;
        fn->u.op.u.call.fun = node->name().c_str();
        info(node)->fn = fn;
        attach(fn);
      }

      VISIT(ReturnNode) {
        node->visitChildren(this);

        FlowNode* fn;
        _pool->alloc(&fn);
        fn->type = FlowNode::RETURN;
        fn->u.op.u.un.op = info(node->returnExpr())->fn->u.op.result;
        info(node)->fn = fn;
        attach(fn);
      }

      template<typename T>
      void literal(AstNode* node, T lit) {
        FlowNode* fn;
        FlowVar *src, *dst;

        _pool->alloc(&fn);
        _pool->alloc(&src);
        _pool->alloc(&dst);

        src->init(lit);

        fn->type = FlowNode::COPY;
        fn->u.op.u.copy.from = src;
        fn->u.op.u.copy.to = fn->u.op.result = dst;
        src->_type = dst->_type = (VarType)info(node)->type;
        info(node)->fn = fn;
        attach(fn);
      }

      VISIT(DoubleLiteralNode) {
        literal(node, node->literal());
      }
    
      VISIT(IntLiteralNode) {
        literal(node, node->literal());
      }

    private:
      CompilerPool* _pool;
      //FlowOrder _order;
      FlowNode* _ncur;
      FlowNode* _first;
    };

    // --------------------------------------------------------------------------------

  private:
    CompilerPool* _pool;
    FlowNode* _first;

    static bool isBranch(AstNode* node) {
      return node->isIfNode() || node->isForNode() || node->isWhileNode();
    }

    VISIT(BinaryOpNode) {
      
    }

    VISIT(UnaryOpNode) {
      node->visitChildren(this);
    }

    VISIT(StringLiteralNode) {
    }

    VISIT(DoubleLiteralNode) {

    }
    
    VISIT(IntLiteralNode) {

    }
    
    VISIT(LoadNode) {
    }
   
    VISIT(StoreNode) {
      node->visitChildren(this);
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
      FlowNode* fn;
      NODE_INFO(node->nodeAt(0))->fn = _pool->allocFlowNode();

      for (unsigned int i = 1; i < node->nodes(); ++i) {
        fn = _pool->allocFlowNode();

        NODE_INFO(node->nodeAt(i))->fn = fn;
        NODE_INFO(node->nodeAt(i - 1))->fn->next = fn;
      }

      //NODE_INFO(node->nodeAt(node->nodes() - 1))->fn->next = NODE_INFO(node)->fn->next;
      //NODE_INFO(node)->fn->next = NODE_INFO(node->nodeAt(0))->fn;
      
      node->visitChildren(this);
    }

    VISIT(PrintNode) {
      size_t args = 0;

      info(node)->fn->type = FlowNode::PRINT;

      for (int i = node->operands() - 1; i >= 0; --i) {
        AstNode* child = node->operandAt(i);

        if (!child->isStringLiteralNode()) {
          child->visit(this);
          
          FlowNode* pushn;
          _pool->alloc(&pushn);
          pushn->type = FlowNode::PUSH;
          pushn->u.op.u.un.op = info(node)->fn->u.op.result;

          args++;
        }
      }

      info(node)->fn->u.print.args = args;
    }

    VISIT(CallNode) {
      node->visitChildren(this);
    }

    VISIT(ReturnNode) {
      node->visitChildren(this);
    }

    void visit(AstFunction* af) {
    }
  };
}
