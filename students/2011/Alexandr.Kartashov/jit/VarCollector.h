#pragma once

#include "common.h"
#include "Runtime.h"

// ================================================================================

namespace mathvm {
  class VarCollector : private AstVisitor {
    typedef std::map<AstVar*, uint16_t> Locals;

  public:
    VarCollector() { 
      _size = 0;
    }

    void collect(AstNode* node, Runtime* rt) {
      _runtime = rt;
      node->visit(this);
    }

  private:
    Runtime* _runtime;
    NativeFunction* _curFun;

  private:
    VISIT(BinaryOpNode) {
      return;
    }

    VISIT(UnaryOpNode) {
      return;
    }

    VISIT(StringLiteralNode) {
      return;
    }

    VISIT(DoubleLiteralNode) {
      return;
    }
    
    VISIT(IntLiteralNode) {
      return;
    }
    
    VISIT(LoadNode) {
      return;
    }
   
    VISIT(StoreNode) {
      return;
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
        fi.next()->visit(this);
      }

      while (vi.hasNext()) {
        v = vi.next();
        VAR_INFO(v) = new VarInfo();
        VAR_INFO(v)->kind = VarInfo::KV_LOCAL;
        VAR_INFO(v)->fPos = _curFun->localsNumber();
        VAR_INFO(v)->owner = _curFun;

        _curFun->setLocalsNumber(_curFun->localsNumber() + 1);
      }
      
      node->visitChildren(this);
    }
    
    VISIT(FunctionNode) {
      NativeFunction* oldFun = _curFun;
      Scope::VarIterator argsIt(af->scope());
      size_t p = 0;

      while (argsIt.hasNext()) {
        AstVar* v = argsIt.next();

        VAR_INFO(v) = new VarInfo();
        VAR_INFO(v)->kind = VarInfo::KV_ARG;
        VAR_INFO(v)->fPos = p;
        VAR_INFO(v)->owner = _curFun;

        ++p;
      }

      _curFun = _runtime->createFunction(node);
      node->setInfo(_curFun);
      node->visitChildren(this);

      _curFun = oldFun;
    }
     
    VISIT(ReturnNode) {
      return;
    }
     
    VISIT(CallNode) {
      return;
    }

    VISIT(PrintNode) {
      return;
    }
  };
}
