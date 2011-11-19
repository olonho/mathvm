#pragma once

#include <sstream>
#include <algorithm>
#include <stack>

#include "common.h"
#include "compiler.h"
#include "Runtime.h"

// ================================================================================

namespace mathvm {
  class VarCollector : private AstVisitor {

  public:
    VarCollector(AstFunction* af, Runtime* rt, CompilerPool* pool) {
      collect(af, rt, pool);
    }

    void collect(AstFunction* af, Runtime* rt, CompilerPool* pool) {
      _parent = NULL;
      _runtime = rt;
      _pool = pool;
      _curBlock = NULL;

      FunctionNode* node = af->node();
      Scope* argScope = af->scope();

      if (argScope) {
        Scope::VarIterator argsIt(argScope);
        size_t p = 0;

        _curFun = info(af->node())->funRef;
        while (argsIt.hasNext()) {
          AstVar* v = argsIt.next();

          _pool->addInfo(v);
          info(v)->initialized = false;
          info(v)->kind = VarInfo::KV_ARG;
          info(v)->fPos = p;
          info(v)->owner = _curFun;
          info(v)->fv->init(v);

          ++p;
        }
      }

      _parent = node;
      node->visitChildren(this);
    }

  private:
    Runtime* _runtime;
    CompilerPool* _pool;
    NativeFunction* _curFun;
    AstNode* _parent;
    BlockNode* _curBlock;

    std::stack<AstNode*> _pstack;

  private:
    void setParent(AstNode* node) {
      info(node)->parent = _parent;
      _pstack.push(_parent);
      _parent = node;
    }

    void restoreParent() {
      _parent = _pstack.top();
      _pstack.pop();
    }

    /*
    void incLocNum() {
    }
    */

    void addLocal(AstVar* v) {
      _pool->addInfo(v);

      info(v)->initialized = false;
      info(v)->kind = VarInfo::KV_LOCAL;
      info(v)->fPos = _curFun->localsNumber();
      info(v)->owner = _curFun;

      _curFun->setLocalsNumber(_curFun->localsNumber() + 1);
    }

    /*
    FlowVar* addLocal() {
      _curFun->setLocalsNumber(_curFun->localsNumber() + 1);
    }
    */
    

    VISIT(BinaryOpNode) {
      _pool->addInfo(node);
      setParent(node);

      node->visitChildren(this);

      info(node)->depth = 
        std::max(
                 info(node->left())->depth, 
                 info(node->right())->depth
                 ) + 1;

      if (NODE_INFO(node->left())->type != NODE_INFO(node->left())->type) {
        ABORT("Type mismatch");
      }

      switch (node->kind()) {
      case tEQ:
      case tNEQ:
      case tGT:
      case tLT:
      case tLE:          
      case tOR:
      case tAND:
        info(node)->type = VAL_INT;
        break;

      default:
        info(node)->type = info(node->left())->type;
        break;
      }

      restoreParent();
    }

    VISIT(UnaryOpNode) {
      _pool->addInfo(node);
      setParent(node);
      
      node->visitChildren(this);

      info(node)->depth = info(node->operand())->depth + 1;
      info(node)->type = info(node->operand())->type;

      restoreParent();
    }

    VISIT(StringLiteralNode) {
      _pool->addInfo(node);
      setParent(node);

      info(node)->depth = 0;

      info(node)->type = VAL_STRING;
      info(node)->string = _runtime->addString(node->literal());

      restoreParent();
    }

    VISIT(DoubleLiteralNode) {
      _pool->addInfo(node);
      setParent(node);
      info(node)->depth = 0;      
      info(node)->type = VAL_DOUBLE;

      restoreParent();
    }
    
    VISIT(IntLiteralNode) {
      _pool->addInfo(node);
      setParent(node);
      info(node)->depth = 0;
      info(node)->type = VAL_INT;

      restoreParent();
    }
    
    VISIT(LoadNode) {
      _pool->addInfo(node);
      setParent(node);
      info(node)->type = (ValType)node->var()->type();
      info(node)->depth = 0;

      restoreParent();
    }
   
    VISIT(StoreNode) {
      _pool->addInfo(node);
      setParent(node);
      
      node->visitChildren(this);
      info(node)->depth = info(node->value())->depth;      

      info(node->var())->initialized = true;

      restoreParent();
    }

    VISIT(ForNode) {
      _pool->addInfo(node);
      setParent(node);
      node->visitChildren(this);
      info(node)->depth = 0;

      addLocal((AstVar*)node->var());      

      restoreParent();
    }

    VISIT(WhileNode) {
      _pool->addInfo(node);
      setParent(node);
      node->visitChildren(this);
      info(node)->depth = 0;

      restoreParent();
    }
    
    VISIT(IfNode) {
      _pool->addInfo(node);
      setParent(node);
      node->visitChildren(this);
      info(node)->depth = 0;

      restoreParent();
    }
          
    VISIT(BlockNode) {
      Scope::VarIterator vi(node->scope());
      AstVar* v;
      BlockNode* oldBlock = _curBlock;

      _curBlock = node;

      _pool->addInfo(node);
      setParent(node);
      info(node)->depth = 0;
      
      while (vi.hasNext()) {
        v = vi.next();
        addLocal(v);
      }
      
      node->visitChildren(this);

      restoreParent();

      _curBlock = oldBlock;
    }

    VISIT(PrintNode) {
      std::stringstream s;
      
      _pool->addInfo(node);
      setParent(node);
      info(node)->depth = 0;
      node->visitChildren(this);
      
      for (size_t i = 0; i < node->operands(); i++) {
        AstNode* n = node->operandAt(i);

        switch (info(n)->type) {
        case VAL_INT:
          s << "%ld";
          break;

        case VAL_STRING:
          s << info(n)->string;
          break;          

        case VAL_DOUBLE:
          s << "%0.0lf";
          break;

        default:
          ABORT("Not supported");
        }
      }

      info(node)->string = _runtime->addString(s.str());

      restoreParent();
    }

    VISIT(CallNode) {
      _pool->addInfo(node);
      setParent(node);
      node->visitChildren(this);

      unsigned int d = 0;
      for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        unsigned int pd = info(node->parameterAt(i))->depth;

        d = (d < pd) ? pd : d;
      }

      info(node)->depth = d;
      info(node)->type = (ValType)_curBlock->scope()->lookupFunction(node->name())->returnType();

      restoreParent();
    }

    VISIT(ReturnNode) {
      _pool->addInfo(node);
      setParent(node);
      info(node)->depth = 0;
      node->visitChildren(this);

      restoreParent();
    }
  };
}
