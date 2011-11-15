#pragma once

#include <deque>
#include <map>
#include <set>
#include <iterator>

#include "compiler.h"

#include "RegAllocator.h"
#include <typeinfo>
// ================================================================================

namespace mathvm {
  static void addRef(FlowNode* from, FlowNode* to) {
    from->refNode = to->refList;
    to->refList = from;
  }

  class Flow : private AstVisitor {

    typedef std::deque<AstNode*> Branches;

  public:
    Flow(AstFunction* root, CompilerPool* pool) {
      FlatPass1 f(root, pool);
      FlatPass2 f2(f.branches(), pool);
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
    /*
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
    */
    
    // --------------------------------------------------------------------------------
    
    class Flattener : protected AstVisitor {
      //typedef std::deque<FlowNode*> FlowOrder;

    public:
      Flattener(CompilerPool* pool) {
        _pool = pool;
        _ncur = NULL;
      }

      FlowNode* first() {
        return _first;
      }

      FlowNode* last() {
        return _ncur;
      }

    protected:
      AstNode* getDecisionNode(AstNode* node) {
        AstNode* p = info(node)->parent;
        TokenKind ptok;

        if (node->isBinaryOpNode()) {
          ptok = node->asBinaryOpNode()->kind();
        }

        for (;;) {
          if (!p->isBinaryOpNode()) {
            break;
          }

          if (p->asBinaryOpNode()->kind() != ptok) {
            break;
          }

          p = info(p)->parent;
        }

        return p;
      }

      void visit(AstNode* node) {
        node->visit(this);
        _ncur->next = NULL;
      }

      void attach(FlowNode* node) {
        if (_ncur) {
          _ncur->next = node;
          node->prev = _ncur;
        } else {
          _first = node;
          _first->next = _first->prev = NULL;
        }

        _ncur = node;
      }

      VISIT(BinaryOpNode) {
        FlowNode* fn;

        node->visitChildren(this);

        if (isLogic(node->kind())) {
          return;
        }

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

        //printf("%s\n", typeid(*node).name());
        
        if (isComp(node->kind())) {
          static FlowNode::Type negCond[] = { FlowNode::GE, FlowNode::GT, FlowNode::LE, FlowNode::GE, FlowNode::NEQ, FlowNode::EQ }; 

          AstNode* p = info(node)->parent;
          FlowNode* to;

          //printf("%s\n", typeid(*p).name());

          if (p->isIfNode()) {
            to = info(info(node)->parent)->trueBranch.begin;

            fn->u.op.trueBranch = to;
            fn->u.op.falseBranch = NULL;
          } else if (p->isBinaryOpNode()) { // parent is an AND or OR operator
            AstNode* dn = getDecisionNode(p);

            switch (p->asBinaryOpNode()->kind()) {
            case tAND: {
              to = info(dn)->falseBranch.begin;

              fn->u.op.falseBranch = to;    // Branch if false
              fn->u.op.trueBranch = NULL;
            }
              break;

            case tOR:
            default:
              ABORT("Not supported yet");
            }
          } else if (p->isUnaryOpNode() && p->asUnaryOpNode()->kind() == tNOT) {
            // Parent is a NOT operator
            p = info(p)->parent;

            AstNode* dn = getDecisionNode(p);
            if (p->isBinaryOpNode()) {
              switch (p->asBinaryOpNode()->kind()) {
              case tAND:
                to = info(dn)->falseBranch.begin;
                
                fn->type = negCond[fn->type - FlowNode::LT];  
                fn->u.op.trueBranch = NULL;      // Branch if true
                fn->u.op.falseBranch = to;
                break;
                
              default:
                ABORT("Not supported");
              }
            } else {
              ABORT("Not supported");
            }
          } else {
            //printf("%s", typeid(*p).name());
            ABORT(" Not supported yet");
          }

          addRef(fn, to);
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

        FlowNode* fn;
        _pool->alloc(&fn);
        info(node)->fn = fn;
        switch (node->kind()) {
        case tNOT:
          return;

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

    protected:
      CompilerPool* _pool;
      //FlowOrder _order;
      FlowNode* _ncur;
      FlowNode* _first;
    };

    // --------------------------------------------------------------------------------

    class FlatPass1 : public Flattener {
    public:
      FlatPass1(AstFunction* root, CompilerPool* pool) 
        : Flattener(pool)
      {
        root->node()->body()->visit(this);
      }

      Branches& branches() {
        return _branches;
      }

      VISIT(IfNode) {
        FlowNode* anchEnd;
        FlowNode* jump;

        _branches.push_back(node);
        insertNop();               // Emit an anchor in case there's two consequetive if's (as in if.mvm)

        _pool->alloc(&anchEnd);
        anchEnd->type = FlowNode::NOP;

        info(node)->last = _ncur;
        node->thenBlock()->visit(this);
        info(node)->trueBranch.begin = info(node)->last->next;
        info(node)->trueBranch.end = _ncur;

        if (node->elseBlock()) {
          _pool->alloc(&jump);
          jump->type = FlowNode::JUMP;
          jump->u.branch = anchEnd;
          attach(jump);

          addRef(jump, anchEnd);
          
          node->elseBlock()->visit(this);
          info(node)->falseBranch.begin = jump->next;
          info(node)->falseBranch.end = _ncur;
        } else {
          info(node)->falseBranch.begin = anchEnd;
        }

        attach(anchEnd);
      }

    private:
      void insertNop() {
        FlowNode* nop;
        _pool->alloc(&nop);
        nop->type = FlowNode::NOP;

        attach(nop);
      }

    private:
      Branches _branches;
    };

    // --------------------------------------------------------------------------------

    /* Compiles branches */
      
    class FlatPass2 : private AstVisitor {
      class SubexpCompiler : public Flattener {
      public:
        SubexpCompiler(AstNode* root, CompilerPool* pool) 
          : Flattener(pool) 
        {
          root->visit(this);
        }
      };

      void insert(SubexpCompiler& comp) {
        _ncur->next->prev = comp.last();
        comp.last()->next = _ncur->next;
        _ncur->next = comp.first();
        comp.first()->prev = _ncur;

        _ncur = comp.last();
      }

      void insert(Block& block) {
        _ncur->next->prev = block.end;
        block.end->next = _ncur->next;
        _ncur->next = block.begin;
        block.begin->prev = _ncur;

        _ncur = block.end;
      }

      void insert(FlowNode* node) {
        node->next = _ncur->next;
        node->prev = _ncur;
        _ncur->next->prev = node;
        _ncur->next = node;

        _ncur = node;
      }

      void move(AstNode* node) {
        _ncur = info(node)->fn;
      }

    public:
      FlatPass2(Branches& branches, CompilerPool* pool) {
        _inIf = false;
        _pool = pool;
        
        for (Branches::iterator it = branches.begin();
             it != branches.end();
             ++it) {
          (*it)->visit(this);
        }

        //root->node()->body()->visit(this);
      }

    private:
      

      VISIT(BinaryOpNode) {
        move(node);
      }

      VISIT(UnaryOpNode) {
        move(node);
      }

      VISIT(StringLiteralNode) {
        move(node);
      }

      VISIT(DoubleLiteralNode) {
        move(node);
      }
    
      VISIT(IntLiteralNode) {
        move(node);
      }
    
      VISIT(LoadNode) {
        move(node);
      }
   
      VISIT(StoreNode) {
        move(node);
      }

      VISIT(ForNode) {
        
      }

      VISIT(WhileNode) {
      }
    
      VISIT(BlockNode) {
        node->visitChildren(this);
      }


      VISIT(ReturnNode) {
        move(node);
      }

      void genIfJump(IfNode* node) {
        FlowNode* jump;

        _pool->alloc(&jump);
        jump->type = FlowNode::JUMP;
        jump->u.branch = info(node)->falseBranch.begin;
        insert(jump);
        addRef(jump, jump->u.branch);
      }

      VISIT(IfNode) {
        _ncur = info(node)->last;

        SubexpCompiler comp(node->ifExpr(), _pool);

        insert(comp);

        if (node->ifExpr()->isBinaryOpNode()) {
          switch (node->ifExpr()->asBinaryOpNode()->kind()) {
          case tAND:      // We don't need the jump after expression if there's AND on the top
            break;        // (we need to insert the false branch before the true branch if
                          // there's anything but AND at the top of the expression)

          default:            
            genIfJump(node);
            break;
          }
        }

        /*
        if (node->elseBlock()) {
          addRef(jump, info(node)->falseBranch.begin);
          jump->u.branch = info(node)->falseBranch.begin;
          insert(jump);
          insert(info(node)->trueBranch);
          
          _pool->alloc(&jump);
          jump->type = FlowNode::JUMP;
          jump->u.branch = oldNext;
          jump->refNode = oldNext->refList;
          oldNext->refList = jump;

          insert(jump);
          insert(info(node)->falseBranch);
        } else {
          jump->u.branch = oldNext;
          insert(jump);
          insert(info(node)->trueBranch);
        }
        */
      }

    private:
      bool _inIf;
      CompilerPool* _pool;
      FlowNode* _ncur;
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
