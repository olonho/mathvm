#pragma once

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

      node->setInfo(_curFun);
      node->visitChildren(this);

      _curFun = oldFun;
    }

    /*
    VISIT(FunctionNode) {
      return;
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
    */
  };
}
