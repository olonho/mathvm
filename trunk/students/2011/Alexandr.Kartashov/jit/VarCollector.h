#pragma once

#include <sstream>
#include <algorithm>

#include "common.h"
#include "compiler.h"
#include "Runtime.h"

// ================================================================================

namespace mathvm {
  class VarCollector : private AstVisitor {
    typedef std::map<AstVar*, uint16_t> Locals;

  public:
    VarCollector() { }

    void collect(AstFunction* af, Runtime* rt, CompilerPool* pool) {
      _runtime = rt;
      _pool = pool;

      FunctionNode* node = af->node();
      Scope::VarIterator argsIt(af->scope());
      size_t p = 0;

      _curFun = info(af->node())->funRef;
      while (argsIt.hasNext()) {
        AstVar* v = argsIt.next();

        _pool->addInfo(v);
        info(v)->initialized = false;
        info(v)->kind = VarInfo::KV_ARG;
        info(v)->fPos = p;
        info(v)->owner = _curFun;

        ++p;
      }

      node->visitChildren(this);
    }

  private:
    Runtime* _runtime;
    CompilerPool* _pool;
    NativeFunction* _curFun;

  private:
    VISIT(BinaryOpNode) {
      _pool->addInfo(node);
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
    }

    VISIT(UnaryOpNode) {
      _pool->addInfo(node);
      
      node->visitChildren(this);

      info(node)->depth = info(node->operand())->depth + 1;
      info(node)->type = info(node->operand())->type;
    }

    VISIT(StringLiteralNode) {
      _pool->addInfo(node);
      info(node)->depth = 0;

      info(node)->type = VAL_STRING;
      info(node)->string = _runtime->addString(node->literal());
    }

    VISIT(DoubleLiteralNode) {
      _pool->addInfo(node);
      info(node)->depth = 0;

      NODE_INFO(node)->type = VAL_DOUBLE;
    }
    
    VISIT(IntLiteralNode) {
      _pool->addInfo(node);
      info(node)->depth = 0;

      NODE_INFO(node)->type = VAL_INT;
    }
    
    VISIT(LoadNode) {
      _pool->addInfo(node);
      info(node)->type = (ValType)node->var()->type();
      info(node)->depth = 0;
    }
   
    VISIT(StoreNode) {
      _pool->addInfo(node);
      
      node->visitChildren(this);
      info(node)->depth = info(node->value())->depth;      

      info(node->var())->initialized = true;
    }

    VISIT(ForNode) {
      _pool->addInfo(node);
      node->visitChildren(this);
      info(node)->depth = 0;
    }

    VISIT(WhileNode) {
      _pool->addInfo(node);
      node->visitChildren(this);
      info(node)->depth = 0;
    }
    
    VISIT(IfNode) {
      _pool->addInfo(node);
      node->visitChildren(this);
      info(node)->depth = 0;
    }
          
    VISIT(BlockNode) {
      Scope::VarIterator vi(node->scope());
      AstVar* v;

      _pool->addInfo(node);
      info(node)->depth = 0;
      
      while (vi.hasNext()) {
        v = vi.next();
        _pool->addInfo(v);
        info(v)->initialized = false;
        info(v)->kind = VarInfo::KV_LOCAL;
        info(v)->fPos = _curFun->localsNumber();
        info(v)->owner = _curFun;

        _curFun->setLocalsNumber(_curFun->localsNumber() + 1);
      }
      
      node->visitChildren(this);
    }

    VISIT(PrintNode) {
      std::stringstream s;
      
      _pool->addInfo(node);
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
    }

    VISIT(CallNode) {
      _pool->addInfo(node);
      node->visitChildren(this);

      unsigned int d = 0;
      for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        unsigned int pd = info(node->parameterAt(i))->depth;

        d = (d < pd) ? pd : d;
      }

      info(node)->depth = d;
    }

    VISIT(ReturnNode) {
      _pool->addInfo(node);
      info(node)->depth = 0;
      node->visitChildren(this);
    }
  };
}
