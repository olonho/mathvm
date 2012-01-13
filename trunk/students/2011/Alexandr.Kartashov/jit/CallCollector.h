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
      _callPoint = NULL;
      _storSize = _maxStor = 0;
      _curBlock = NULL;

      root->node()->body()->visit(this);

      info(root->node())->funRef->setCallStorageSize(_maxStor);
    }

    size_t storSize() const {
      return _storSize;
    }

  private:
    void saveCallPoint(AstNode* node) {
      _oldCallPoint = _callPoint;
      _callPoint = node;
    }

    void restoreCallPoint() {
      _callPoint = _oldCallPoint;
    }

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
      saveCallPoint(node->whileExpr());
      node->visitChildren(this);
      restoreCallPoint();
      updateCallStorage(node);
    }
    
    VISIT(BlockNode) {
      BlockNode* oldBlock = _curBlock;
      _curBlock = node;

      saveCallPoint(NULL);
      for (uint32_t i = 0; i < node->nodes(); ++i) {
        _callPoint = node->nodeAt(i);
        node->nodeAt(i)->visit(this);
      }
      restoreCallPoint();

      //node->visitChildren(this);
      updateCallStorage(node);

      _curBlock = oldBlock;
    }

    VISIT(ReturnNode) {
      node->visitChildren(this);
      updateCallStorage(node);
    }

    VISIT(IfNode) {
      saveCallPoint(node);
      _callPoint = node->ifExpr();
      node->visitChildren(this);
      restoreCallPoint();
      updateCallStorage(node);
    }

    VISIT(CallNode) {
      info(node)->callList = info(_callPoint)->callList;
      info(_callPoint)->callList = node;

      if (!info(node)->parent->isBlockNode()) {
        if (!_caller) {
          _caller = info(node)->parent;
        }
        
        /*
        info(node)->callList = info(_caller)->callList;
        info(_caller)->callList = node;
        */

        saveCallPoint(NULL);
        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
          _callPoint = node->parameterAt(i);
          node->parameterAt(i)->visit(this);
        }
        restoreCallPoint();

        //node->visitChildren(this);

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

    AstNode* _callPoint;
    AstNode* _oldCallPoint;

    size_t _storSize;
    size_t _maxStor;
  };
}
