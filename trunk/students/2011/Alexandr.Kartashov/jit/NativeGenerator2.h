#pragma once

#include "common.h"
#include "Runtime.h"

// ================================================================================

namespace mathvm {
  #define VAR_SIZE 8

  static char rmap[] = {RCX, RDX, RBX, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15};

  class NativeGenerator2 {
    
  public:
    NativeGenerator2(AstFunction* af) {
      X86Code* code = info(af->node())->funRef->code();
      size_t nargs = 0;
      size_t locNum = info(af->node())->funRef->localsNumber();

      code->mov_rr(RAX, RSP);
      
      code->push_r(RBP);
      code->mov_rr(RBP, RSP);

      if (locNum % 2 == 0) {   // Stack frame alignment (I'd like to kill the person who suggested this :(
        locNum++;              // We have RBP and RDI stored and odd number of locals
      }

      code->sub_rm_imm(RSP, locNum*VAR_SIZE);

      if (af->name() == AstFunction::top_name) {
        Scope::VarIterator vi(af->node()->body()->scope());
        
        while (vi.hasNext()) {
          vi.next();
          nargs++;
        }

        code->push_r(RDI);
        code->mov_rr(RSI, RDI);
        code->mov_rr(RDI, RBP);
        code->sub_rm_imm(RDI, nargs*VAR_SIZE);
        code->mov_r_imm(RCX, nargs);
        code->add(REPNE);
        code->add(x86_rex(0, 0, 1));
        code->add(MOVS_W);        
      }

      _code = code;
      generate(info(af->node())->fn);

      if (af->name() == AstFunction::top_name) {
        code->pop_r(RDI);
        code->mov_rr(RSI, RBP);
        code->sub_rm_imm(RSI, nargs*VAR_SIZE);

        code->mov_r_imm(RCX, nargs);
        code->add(REPNE);
        code->add(x86_rex(0, 0, 1));
        code->add(MOVS_W);
      }

      code->add_rm_imm(RSP, locNum*VAR_SIZE);
      code->pop_r(RBP);
      code->add(RET);      
    }


  private:    
    void generate(FlowNode* node) {
      for (; node; node = node->next) {
        node->offset = _code->current();

        if (node->refList) {
          for (FlowNode* ref = node->refList; ref; ref = ref->refNode) {
            if (ref->offset != INVALID_OFFSET) {
              ref->label.bind();
            }
          }
        }

        switch (node->type) {
        case FlowNode::LT:
        case FlowNode::LE:
        case FlowNode::GT:
        case FlowNode::GE:
        case FlowNode::EQ:
        case FlowNode::NEQ:
          genComp(node);
          break;

        case FlowNode::COPY:
          genCopy(node);
          break;

        case FlowNode::ADD:
          genAdd(node);
          break;

        case FlowNode::SUB:
          genSub(node);
          break;

        case FlowNode::MUL:
          genMul(node);
          break;

        case FlowNode::NEG:
          genNeg(node);
          break;

        case FlowNode::INC:
          genInc(node);
          break;

        case FlowNode::PRINT:
          genPrint(node);
          break;

        case FlowNode::PUSH:
          genPush(node);
          break;

        case FlowNode::DIV:
          genDiv(node);
          break;

        case FlowNode::NOP:
          break;

        case FlowNode::JUMP:
          genJump(node);
          break;

        default:
          ABORT("Not supported");          
        }
      }
    }

#define INT_OP(op)                              \
    switch (dst->_stor) {                                               \
    case FlowVar::STOR_REGISTER:                                        \
      switch (src->_stor) {                                             \
      case FlowVar::STOR_CONST:                                         \
        _code->op(ireg(dst), Imm(src->_const.intConst));                \
        break;                                                          \
                                                                        \
      case FlowVar::STOR_LOCAL:                                         \
        _code->op(ireg(dst), mem(src));                                 \
        break;                                                          \
                                                                        \
      case FlowVar::STOR_REGISTER:                                      \
        _code->op(ireg(dst), ireg(src));                                \
        break;                                                          \
                                                                        \
      default:                                                          \
        ABORT("Not supported");                                         \
      }                                                                 \
      break;                                                            \
                                                                        \
    case FlowVar::STOR_LOCAL:                                           \
      switch (src->_stor) {                                             \
      case FlowVar::STOR_REGISTER:                                      \
        _code->op(mem(dst), ireg(src));                                 \
        break;                                                          \
                                                                        \
      case FlowVar::STOR_TEMP:                                          \
        _code->op(mem(dst), Reg(RAX));                                  \
        break;                                                          \
                                                                        \
      default:                                                          \
        ABORT("Not supported");                                         \
        }                                                               \
      break;                                                            \
                                                                        \
    case FlowVar::STOR_TEMP:                                            \
      switch (src->_stor) {                                             \
      case FlowVar::STOR_LOCAL:                                         \
        _code->op(Reg(RAX), mem(src));                                  \
        break;                                                          \
                                                                        \
      default:                                                          \
        ABORT("Not supported");                                         \
      }                                                                 \
      break;                                                            \
                                                                        \
    default:                                                            \
      ABORT("Not supported");                                           \
    }


#define DOUBLE_OP(op)                                                   \
    switch (dst->_stor) {                                               \
    case FlowVar::STOR_REGISTER:                                        \
      switch (src->_stor) {                                             \
      case FlowVar::STOR_CONST:                                         \
        _code->mov(Reg(RAX), Imm(src->_const.intConst));                \
        _code->op(XmmReg(dst->_storIdx), Reg(RAX));                     \
        break;                                                          \
                                                                        \
      case FlowVar::STOR_LOCAL:                                         \
        _code->op(XmmReg(dst->_storIdx), mem(src));                     \
        break;                                                          \
                                                                        \
      case FlowVar::STOR_REGISTER:                                      \
        _code->op(XmmReg(dst->_storIdx), XmmReg(src->_storIdx));        \
        break;                                                          \
                                                                        \
      default:                                                          \
        ABORT("Not supported");                                         \
        }                                                               \
      break;                                                            \
                                                                        \
    case FlowVar::STOR_LOCAL:                                           \
      switch (src->_stor) {                                             \
      case FlowVar::STOR_REGISTER:                                      \
        _code->op(mem(dst), XmmReg(src->_storIdx));                     \
        break;                                                          \
                                                                        \
      default:                                                          \
        ABORT("Not supported");                                         \
      }                                                                 \
      break;                                                            \
                                                                        \
    default:                                                            \
      ABORT("Not supported");                                           \
    }

#define DOUBLE_OP2(op, arg)                                             \
    switch (dst->_stor) {                                               \
    case FlowVar::STOR_REGISTER:                                        \
      switch (src->_stor) {                                             \
      case FlowVar::STOR_CONST:                                         \
        _code->mov(Reg(RAX), Imm(src->_const.intConst));                \
        _code->op(XmmReg(dst->_storIdx), Reg(RAX), arg);                \
        break;                                                          \
                                                                        \
      case FlowVar::STOR_LOCAL:                                         \
        _code->op(XmmReg(dst->_storIdx), mem(src));                     \
        break;                                                          \
                                                                        \
      case FlowVar::STOR_REGISTER:                                      \
        _code->op(XmmReg(dst->_storIdx), XmmReg(src->_storIdx), arg);   \
        break;                                                          \
                                                                        \
      default:                                                          \
        ABORT("Not supported");                                         \
      }                                                                 \
      break;                                                            \
                                                                        \
    case FlowVar::STOR_LOCAL:                                           \
      switch (src->_stor) {                                             \
      case FlowVar::STOR_REGISTER:                                      \
        _code->op(mem(dst), XmmReg(src->_storIdx), arg);                \
        break;                                                          \
                                                                        \
      default:                                                          \
        ABORT("Not supported");                                         \
      }                                                                 \
      break;                                                            \
                                                                        \
    default:                                                            \
      ABORT("Not supported");                                           \
    }
    

#define BIN_OP(op)                                                      \
    switch (src->_type) {                                               \
    case VT_INT:                                                        \
      INT_OP(op)                                                        \
      break;                                                            \
                                                                        \
    case VT_DOUBLE:                                                     \
      DOUBLE_OP(op)                                                     \
      break;                                                            \
                                                                        \
    default:                                                            \
      ABORT("Not supported");                                           \
    }


  private:

  void genCopy(FlowNode* node) {
    FlowVar* src = node->u.op.u.copy.from;
    FlowVar* dst = node->u.op.u.copy.to;

    BIN_OP(mov);
  }

  void genAdd(FlowNode* node) {
    FlowVar* src = node->u.op.u.bin.op2;
    FlowVar* dst = node->u.op.u.bin.op1;

    BIN_OP(add);
  }

  void genSub(FlowNode* node) {
    FlowVar* src = node->u.op.u.bin.op2;
    FlowVar* dst = node->u.op.u.bin.op1;

    BIN_OP(sub);
  }

  void genMul(FlowNode* node) {
    FlowVar* src = node->u.op.u.bin.op2;
    FlowVar* dst = node->u.op.u.bin.op1;

    BIN_OP(mul);
  }

  void genDiv(FlowNode* node) {
    FlowVar* src = node->u.op.u.bin.op2;
    FlowVar* dst = node->u.op.u.bin.op1;

    BIN_OP(div);
  }

  void genNeg(FlowNode* node) {
    FlowVar* op = node->u.op.u.un.op;

    switch (op->_type) {
    case VT_INT:
      switch (op->_stor) {
      case FlowVar::STOR_REGISTER:
        _code->neg_r(rmap[op->_storIdx]);
        break;

      default:
        ABORT("Not supported");
      }
      break;

    case VT_DOUBLE:
      switch (op->_stor) {
      case FlowVar::STOR_REGISTER:
        _code->sub_r_imm(RSP, sizeof(double));
        _code->movq_m_xmm(op->_storIdx, RSP, 0);
        _code->sub_xmm_m(op->_storIdx, RSP, 0);
        _code->sub_xmm_m(op->_storIdx, RSP, 0);
        _code->add_r_imm(RSP, sizeof(double));
        break;

      default:
        ABORT("Not supported");
      }
      break;

    default:
      ABORT("Not supported");
    }
  }


  void genPush(FlowNode* node) {
    FlowVar* op = node->u.op.u.un.op;

    switch (op->_type) {
    case VT_INT:
      switch (op->_stor) {
      case FlowVar::STOR_REGISTER:
        _code->push_r(rmap[op->_storIdx]);
        break;

      default:
        ABORT("Not supported");
      }
      break;

    case VT_DOUBLE:
      switch (op->_stor) {
      case FlowVar::STOR_REGISTER:
        _code->sub_r_imm(RSP, sizeof(double));
        _code->movq_m_xmm(op->_storIdx, RSP, 0);
        break;

      default:
        ABORT("Not supported");
      }
      break;

    default:
      ABORT("Not supported");
    }
  }

  void genPrint(FlowNode* node) {
    static const char iamap[] = { RDI, RSI, RDX, RCX, R8, R9 };      

    PrintNode* pn = node->u.print.ref;
    size_t iarg = 1, darg = 0;
    size_t carg = 0;

    for (size_t i = 0; i < pn->operands(); ++i) {
      AstNode* child = pn->operandAt(i);

      switch (info(child)->type) {
      case VAL_INT:
        _code->mov_rm(iamap[iarg], RSP, VAR_SIZE*(node->u.print.args - carg - 1));
        iarg++;
        carg++;
        break;

      case VAL_DOUBLE:
        _code->movq_xmm_m(darg, RSP, VAR_SIZE*(node->u.print.args - carg - 1));
        darg++;
        carg++;
        break;

      default:
        break;
      }
    }

    //const char* s = "%lf\n";

    _code->mov_r_imm(RDI, (size_t)info(pn)->string);
    _code->mov_r_imm(RAX, darg);
    _code->add_r_imm(RSP, VAR_SIZE*node->u.print.args);
    _code->mov_r_imm(RBX, (uint64_t)&printf);
    _code->call_r(RBX);
  }

  void genJump(FlowNode* node) {
    node->label = _code->jmp_rel32();

    if (node->u.op.trueBranch && node->u.op.trueBranch->offset != INVALID_OFFSET) {
      node->label.bind(node->u.op.trueBranch->offset);
    } else if (node->u.op.falseBranch && node->u.op.falseBranch->offset != INVALID_OFFSET) {
      node->label.bind(node->u.op.falseBranch->offset);
    } else if (node->u.branch && node->u.branch->offset != INVALID_OFFSET) {
      node->label.bind(node->u.branch->offset);
    }
  }

  void genComp(FlowNode* node) {
    static char posCond[] = { CC_L,  CC_LE,  CC_G,  CC_GE,  CC_E,  CC_NE };
    static char negCond[] = { CC_NL, CC_NLE, CC_NG, CC_NGE, CC_NE, CC_E  };

    FlowVar* src = node->u.op.u.bin.op2;
    FlowVar* dst = node->u.op.u.bin.op1;
    char cond;

    switch (src->_type) {
    case VT_INT:
      INT_OP(cmp);

      if (node->u.op.trueBranch) {
        cond = posCond[node->type - FlowNode::LT];
      } else {
        cond = negCond[node->type - FlowNode::LT];
      }

      node->label = _code->jcc_rel32(cond);

      /*
      switch (node->type) {
      case FlowNode::LT:
        node->label = _code->jcc_rel32(CC_L);
        break;

      case FlowNode::LE:
        node->label = _code->jcc_rel32(CC_LE);
        break;

      case FlowNode::GT:
        node->label = _code->jcc_rel32(CC_G);
        break;

      case FlowNode::GE:
        node->label = _code->jcc_rel32(CC_GE);
        break;

      case FlowNode::EQ:
        node->label = _code->jcc_rel32(CC_E);
        break;

      case FlowNode::NEQ:
        node->label = _code->jcc_rel32(CC_NE);
        break;

      default:
        ABORT("Should never happen");
      }
      */

      break;

    case VT_DOUBLE:
      char arg;

      arg = 0;

      switch (node->type) {
      case FlowNode::LT:
        arg = CC_L;
        break;

      case FlowNode::LE:
        arg = CC_LE;
        break;

      case FlowNode::GT:
        arg = CC_G;
        break;

      case FlowNode::GE:
        arg = CC_GE;
        break;

      case FlowNode::EQ:
        arg = CC_E;
        break;

      case FlowNode::NEQ:
        arg = CC_NE;
        break;

      default:
        ABORT("Should never happen");
      }

      DOUBLE_OP2(cmp, arg);
      break;

    default:
      ABORT("Should never happen");
    }

    if (node->u.op.trueBranch && node->u.op.trueBranch->offset != INVALID_OFFSET) {
      node->label.bind(node->u.op.trueBranch->offset);
    }
  }

  void genInc(FlowNode* node) {
    FlowVar* op = node->u.op.u.un.op;

    switch (op->_type) {
    case VT_INT:
      switch (op->_stor) {
      case FlowVar::STOR_LOCAL:
        _code->inc(mem(op));
        break;

      default:
        ABORT("Not supported");
      }
      break;

    default:
      ABORT("Not supported");
    }
  }

  static Mem mem(FlowVar* fv) {
    switch (fv->_stor) {
    case FlowVar::STOR_LOCAL:
      return Mem(RBP, -VAR_SIZE*fv->_vi->fPos - 8);
      break;

    default:
      ABORT("Not supported yet");
    }
  }

  static Reg ireg(FlowVar* fv) {
    return Reg(rmap[fv->_storIdx]);
  }
    
  private:
    X86Code* _code;
  };
}
