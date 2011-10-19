#pragma once

#include "common.h"

// ================================================================================

namespace mathvm {
  class LocalCollector : private AstVisitor {
    typedef std::map<AstVar*, uint16_t> Locals;

    VarNum* _varMap;
    size_t _size;


  public:
    LocalCollector() { 
      _size = 0;
    }

    void collectLocals(VarNum* varMap, AstNode* node) {
      _varMap = varMap;
      node->visit(this);
    }

    size_t size() const {
      return _size;
    }

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
      AstVar *v;

      while (vi.hasNext()) {
        v = vi.next();
        _varMap->add(v);
        _size++;
      }
      
      node->visitChildren(this);
    }
    
    VISIT(FunctionNode) {
      node->visitChildren(this);
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
