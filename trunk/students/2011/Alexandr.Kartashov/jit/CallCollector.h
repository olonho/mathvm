#include "common.h"
#include "compiler.h"

// ================================================================================

namespace mathvm {

  // Collects function calls

  class CallCollector : private AstVisitor {
  public:
    CallCollector(AstFunction* root, CompilerPool* pool) {
      _pool = pool;
      _caller = NULL;
      _storSize = _maxStor = 0;
      _curBlock = NULL;

      root->node()->body()->visit(this);

      info(root->node())->funRef->setCallStorageSize(_maxStor);
    }

    size_t storSize() const {
      return _storSize;
    }

  private:

    void updateCallStorage(AstNode* node) {
      if (node == _caller) {
        if (_storSize > _maxStor) {
          _maxStor = _storSize;
          _storSize = 0;
        }

        _caller = NULL;
      }
    }

    VISIT(BinaryOpNode) {
      node->visitChildren(this);
      updateCallStorage(node);
    }

    VISIT(UnaryOpNode) {
      node->visitChildren(this);
      updateCallStorage(node);
    }

    VISIT(StoreNode) {
      node->visitChildren(this);
      updateCallStorage(node);
    }

    VISIT(ForNode) {
      node->visitChildren(this);
      updateCallStorage(node);
    }

    VISIT(WhileNode) {
      node->visitChildren(this);
      updateCallStorage(node);
    }
    
    VISIT(BlockNode) {
      BlockNode* oldBlock = _curBlock;
      _curBlock = node;

      node->visitChildren(this);
      updateCallStorage(node);

      _curBlock = oldBlock;
    }

    VISIT(ReturnNode) {
      node->visitChildren(this);
      updateCallStorage(node);
    }

    VISIT(IfNode) {
      node->visitChildren(this);
      updateCallStorage(node);
    }

    VISIT(CallNode) {
      if (!info(node)->parent->isBlockNode()) {
        if (!_caller) {
          _caller = info(node)->parent;
        }

        info(node)->callList = info(_caller)->callList;
        info(_caller)->callList = node;

        node->visitChildren(this);

        FlowVar* res;
        VarInfo* vi;
        _pool->alloc(&res);
        _pool->alloc(&vi);
        vi->kind = VarInfo::KV_LOCAL;
        vi->fPos = _storSize;
        res->_type = _curBlock->scope()->lookupFunction(node->name())->returnType();
        res->_stor = FlowVar::STOR_CALL;
        res->_vi = vi;

        info(node)->callRes = res;

        _storSize++;
      } else {
        // Parent is a block node --- no result is required

        node->visitChildren(this);
        updateCallStorage(node);
      }
    }

    VISIT(PrintNode) {
      node->visitChildren(this);
      updateCallStorage(node);
    }

  private:
    CompilerPool* _pool;
    BlockNode* _curBlock;
    AstNode* _caller;
    size_t _storSize;
    size_t _maxStor;
  };
}
