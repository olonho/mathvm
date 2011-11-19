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

  template<typename T>
  class NativeLabel {
  public:    
    NativeLabel() { }

    NativeLabel(Bytecode* bc) {
      _label = bc->current();
      bc->addTyped<T>(0);
      _bc = bc;
    }

    void bind(uint32_t offset) {
      int32_t d = (int32_t)offset - (int32_t)_label - sizeof(T);
      _bc->setTyped<int32_t>(_label, d);
    }

    void bind(Bytecode* bc) {
      bind(bc->current());
    }

    void bind(const NativeLabel& label) {
      int32_t d = (int32_t)label._label - (int32_t)_label - sizeof(T);
      _bc->setTyped<int32_t>(_label, d);
    }

    void bind() {
      bind(_bc);
    }

    uint32_t offset() const {
      return _label;
    }

  private:
    uint32_t _label;
    Bytecode* _bc;
  };
  
  // --------------------------------------------------------------------------------

  class FlowVar;

  class NativeFunction;

  struct VarInfo {
    enum Kind {
      KV_LOCAL,
      KV_CALL_STOR,
      KV_ARG
    };
    
    Kind kind;
    int fPos;
    NativeFunction* owner;
    FlowVar* fv;
    bool initialized;
  };
  
  class FlowVar {
  public:
    enum Storage {
      STOR_INVALID,
      STOR_REGISTER,  // _storIdx --- the register assinged to the variable
      STOR_SPILL,     // _storIdx --- the index of the frame bucket
      STOR_CONST,     // _const is valid
      STOR_LOCAL,     // _avar is valid
      STOR_TEMP,      // RAX :)
      STOR_CALL,
      STOR_ARG
    };

    FlowVar() {
      _type = VT_INVALID;
      _avar = NULL;
      _stor = STOR_INVALID;
    }

    void init(const AstVar* v) {
      _type = v->type();
      _avar = v;

      switch (((VarInfo*)v->info())->kind) {
      case VarInfo::KV_LOCAL:
        _stor = STOR_LOCAL;
        break;

      case VarInfo::KV_ARG:
        _stor = STOR_ARG;
        break;

      default:
        break;
      }

      _vi = (VarInfo*)v->info();
    }

    void init(int64_t v) {
      _type = VT_INT;
      _const.intConst = v;
      _stor = STOR_CONST;
      _avar = NULL;
    }

    void init(double v) {
      _type = VT_DOUBLE;
      _const.doubleConst = v;
      _stor = STOR_CONST;
      _avar = NULL;
    }

    VarType _type;
    const AstVar*  _avar;
    const VarInfo* _vi;
    const FlowVar* _nextArg;
    const FlowVar* _prevArg;

    union {
      int64_t intConst;
      double doubleConst;
    } _const;

    Storage _stor;
    unsigned int _storIdx;
  };

  // --------------------------------------------------------------------------------

  #define INVALID_OFFSET 0xFFFFFFFF
  
  struct FlowNode {
    enum Type {
      ASSIGN,
      CONST,
      COPY,
      PUSH,

      ADD,
      SUB,
      MUL,
      DIV,
      NEG,
      INC,

      LT,
      LE,
      GT,
      GE,
      EQ,
      NEQ,

      AND,
      OR,
      NOT,

      SYS_CALL,
      CALL,
      PRINT,
      RETURN,
      JUMP,

      ALIGN,
      UNALIGN,

      NOP
    };

    Type type;
    FlowNode* next;
    FlowNode* prev;

    uint32_t offset;

    FlowNode* refList;
    FlowNode* refNode;

    NativeLabel<int32_t> label;

    union {
      struct {
        FlowVar* result;
        FlowNode* trueBranch;
        FlowNode* falseBranch;

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
            AstFunction* af;
            //NativeFunction* fun;
          } call;

          struct {
            FlowVar* from;
            FlowVar* to;
          } copy;
        } u;        
      } op;

      struct {
        FlowVar* to;
        FlowVar* from;
      } assign;


      /*struct { 
        
        } condBranch;*/

      FlowNode* branch;

      struct {
        size_t args;
        PrintNode* ref;
      } print;

      struct {
        FlowVar* firstArg;
      } sysCall;
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

  struct Block {
    FlowNode* begin;
    FlowNode* end;
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

  struct NodeInfo {
    ValType type;
    unsigned int depth;     // Size of the subexpression tree
    AstNode* parent;        // The parent of the node

    FlowNode* last;         // the last flattened node before this one
                            // (required to compile branching AST nodes)

    /* I wish I had anonymous unions.... */
    
    //FlowVar* lastVal;       // the last value of the for-cycle

    /* I forgot, OH SHI~ */

    NativeFunction* funRef;

    /* For an operation node */

    FlowNode* fn;

    /* For a print node */

    const char* string;
    
    /* For a branching node */

    Block trueBranch;
    Block falseBranch;

    /* Calls to be done before the expresion is evaluated */

    CallNode* callList;

    /* Call result for call node */

    FlowVar* callRes;
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
    case tNOT:
      return true;

    default:
      return false;
    }
  }
}
