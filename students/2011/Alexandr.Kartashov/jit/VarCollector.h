#pragma once

#include <sstream>

#include "common.h"
#include "Runtime.h"

// ================================================================================

namespace mathvm {
  class VarCollector : private AstVisitor {
    typedef std::map<AstVar*, uint16_t> Locals;

  public:
    VarCollector() { }

    void collect(AstFunction* af, Runtime* rt) {
      _runtime = rt;
      visit(af);
    }

  private:
    Runtime* _runtime;
    NativeFunction* _curFun;

  private:
    VISIT(BinaryOpNode) {
      node->visitChildren(this);

      if (NODE_INFO(node->left())->type != NODE_INFO(node->left())->type) {
        ABORT("Type mismatch");
      }

      node->setInfo(new NodeInfo);
      switch (node->kind()) {
      case tEQ:
      case tNEQ:
      case tGT:
      case tLT:
      case tLE:          
      case tOR:
      case tAND:
        NODE_INFO(node)->type = VAL_INT;
        break;

      default:
        NODE_INFO(node)->type = NODE_INFO(node->left())->type;
        break;
      }
    }

    VISIT(UnaryOpNode) {
      node->visitChildren(this);

      node->setInfo(new NodeInfo);
      NODE_INFO(node)->type = NODE_INFO(node->operand())->type;
    }

    VISIT(StringLiteralNode) {
      node->setInfo(new NodeInfo);
      NODE_INFO(node)->type = VAL_STRING;
      NODE_INFO(node)->string = _runtime->addString(node->literal());
    }

    VISIT(DoubleLiteralNode) {
      node->setInfo(new NodeInfo);
      NODE_INFO(node)->type = VAL_DOUBLE;
    }
    
    VISIT(IntLiteralNode) {
      node->setInfo(new NodeInfo);
      NODE_INFO(node)->type = VAL_INT;
    }
    
    VISIT(LoadNode) {
      node->setInfo(new NodeInfo);
      NODE_INFO(node)->type = (ValType)node->var()->type();
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
      Scope::VarIterator vi(node->scope());
      Scope::FunctionIterator fi(node->scope());
      AstVar* v;
      
      while (fi.hasNext()) {
        visit(fi.next());
      }

      while (vi.hasNext()) {
        v = vi.next();
        v->setInfo(new VarInfo());
        VAR_INFO(v)->kind = VarInfo::KV_LOCAL;
        VAR_INFO(v)->fPos = _curFun->localsNumber();
        VAR_INFO(v)->owner = _curFun;

        _curFun->setLocalsNumber(_curFun->localsNumber() + 1);
      }
      
      node->visitChildren(this);
    }

    VISIT(PrintNode) {
      std::stringstream s;
      
      node->setInfo(new NodeInfo);
      node->visitChildren(this);
      
      for (size_t i = 0; i < node->operands(); i++) {
        AstNode* n = node->operandAt(i);

        switch (NODE_INFO(n)->type) {
        case VAL_INT:
          s << "%ld";
          break;

        case VAL_STRING:
          s << NODE_INFO(n)->string;
          break;          

        case VAL_DOUBLE:
          s << "%0.0lf";
          break;

        default:
          ABORT("Not supported");
        }
      }

      NODE_INFO(node)->string = _runtime->addString(s.str());
    }

    VISIT(CallNode) {
      node->visitChildren(this);
    }

    VISIT(ReturnNode) {
      node->visitChildren(this);
    }

    void visit(AstFunction* af) {
      NativeFunction* oldFun = _curFun;
      FunctionNode* node = af->node();
      Scope::VarIterator argsIt(af->scope());
      size_t p = 0;

      _curFun = _runtime->createFunction(af);

      while (argsIt.hasNext()) {
        AstVar* v = argsIt.next();

        v->setInfo(new VarInfo());
        VAR_INFO(v)->kind = VarInfo::KV_ARG;
        VAR_INFO(v)->fPos = p;
        VAR_INFO(v)->owner = _curFun;

        ++p;
      }

      node->setInfo(new NodeInfo);
      NODE_INFO(node)->funRef = _curFun;
      node->visitChildren(this);

      _curFun = oldFun;
    }
  };
}
