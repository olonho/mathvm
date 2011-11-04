#pragma once

#include <cstdio>
#include <cstdlib>

#include <deque>

#include "ast.h"
#include "mathvm.h"

// ================================================================================

#define VISIT(type)                             \
  void visit##type(type* node)


#define ABORT(x...)                             \
  printf(x);                                    \
  abort();

// --------------------------------------------------------------------------------

namespace mathvm {

  template<typename T>
  class Pool {
  public:
    T* alloc() {
      _d.push_back(T());
      return &_d.back();
    }
    
  private:
    std::deque<T> _d;
  };
  
  // --------------------------------------------------------------------------------
  
  class FlowVar {
  public:
    enum Storage {
      STOR_REGISTER,
      STOR_STACK
    };

    FlowVar() {
      _type = VT_INVALID;
      _avar = NULL;
    }

    void init(const AstVar* v) {
      _type = v->type();
      _avar = v;
    }

    void init(int64_t v) {
      _type = VT_INT;
      _const.intConst = v;
    }

    void init(double v) {
      _type = VT_DOUBLE;
      _const.doubleConst = v;
    }

    VarType _type;
    const AstVar* _avar;

    union {
      int64_t intConst;
      double doubleConst;
    } _const;

    Storege stor;
    unsigned int storIdx;
  };

  // --------------------------------------------------------------------------------

  struct FlowNode {
    enum Type {
      ASSIGN,
      CONST,
      PUSH,

      ADD,
      SUB,
      MUL,
      DIV,
      NEG,

      LT,
      LE,
      GT,
      GE,
      EQ,
      NEQ,
      AND,
      OR,
      NOT,

      CALL,
      PRINT,
      RETURN
    };

    Type type;
    FlowNode* next;

    union {
      struct {
        FlowVar* result;

        union {
          struct {
            FlowVar* op1;
            FlowVar* op2;
          } bin;

          struct {
            FlowVar* op;
          } un;

          struct {
            const char* fun;
          } call;
        } u;        
      } op;

      struct {
        FlowVar* to;
        FlowVar* from;
      } assign;

      struct { 
        FlowVar* op1;
        FlowVar* op2;

        FlowNode* trueBrach;
        FlowNode* falseBranch;
      } localBranch;

      struct {
        PrintNode* ref;
      } print;
    } u;

    bool isAssign() const {
      return type == ASSIGN;
    }

    bool isArith() const {
      return type >= ADD && type <= NEG;
    }

    bool isBranch() const {
      return type >= LT && type <= NOT;
    }
  };

  // --------------------------------------------------------------------------------

  class NativeFunction;

  enum ValType {
    VAL_INVALID = 0,
    VAL_VOID,
    VAL_DOUBLE,
    VAL_INT,
    VAL_STRING,
    VAL_BOOL
  };

  struct VarInfo {
    enum Kind {
      KV_LOCAL,
      KV_ARG
    };
    
    Kind kind;
    int fPos;
    NativeFunction* owner;
    FlowVar* fv;
    bool init;
  };

  struct NodeInfo {
    ValType type;
    NativeFunction* funRef;
    const char* string;
    FlowNode* fn;
    unsigned int depth;
  };

#define VAR_INFO(v) ((VarInfo*)(v->info()))
#define NODE_INFO(n) ((NodeInfo*)(n->info()))

  static NodeInfo* info(const AstNode* node) {
    return NODE_INFO(node);
  }

  static VarInfo* info(const AstVar* var) {
    return VAR_INFO(var);
  }

  // --------------------------------------------------------------------------------
  // Token classifiers

  static bool isComp(TokenKind tok) {
    switch (tok) {
    case tEQ:
    case tNEQ:  
    case tLT:
    case tLE:
    case tGT:
    case tGE:
      return true;

    default:
      return false;
    }
  }

  static bool isArith(TokenKind tok) {
    switch (tok) {
    case tADD:
    case tSUB:  
    case tMUL:
    case tDIV:
      return true;

    default:
      return false;
    }
  }

  static bool isLogic(TokenKind tok) {
    switch (tok) {
    case tAND:
    case tOR:
      return true;

    default:
      return false;
    }
  }
}
