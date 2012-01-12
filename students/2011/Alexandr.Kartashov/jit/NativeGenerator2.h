#pragma once

#include "common.h"
#include "FunctionCollector.h"

// ================================================================================

namespace mathvm {
  #define VAR_SIZE 8

  /* The mapping from abstract registers to machine registers */

  static char rmap[] = {RCX, RDX, RBX, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15};

  /* Native function argument placement */

  static const char iamap[] = { RDI, RSI, RDX, RCX, R8, R9 };


  /**
   *  The stack frame structure:
   *
   *  [RBP] --- the previous frame pointer
   *  [RBP - 8] --- the pointer to the current NativeFunction
   *  [RBP - 16] --- old RDI (for the top function only)
   *  [RBP - 24(16)] --- locals
   *  [Temparary call storage]
   */

  class NativeGenerator2 {
    
  public:
    NativeGenerator2(AstFunction* af) {
      _curf = info(af->node())->funRef;

      X86Code* code = _curf->code();
      size_t nargs = 0;
      size_t locNum = _curf->localsNumber() + _curf->externVars() + _curf->callStorageSize();      
      bool isTop = af->name() == AstFunction::top_name;
      bool noArgs;

      if (!isTop) {
        noArgs = af->parametersNumber() == 0;
      } else {
        noArgs = _curf->localsNumber() == 0;
      }

      _localsBase = 2;                                            // RBP + [current function]

      _extBase = _localsBase + _curf->localsNumber();
      _callBase = _extBase + _curf->externVars();
      
      code->push_r(RBP);
      code->mov_rr(RBP, RSP);

      code->mov_r_imm(RAX, (uint64_t)_curf);
      code->push_r(RAX);

      if ((!isTop || (isTop && noArgs)) && locNum % 2 == 0) {   // Stack frame alignment (I'd like to kill the person who suggested this :(
        locNum++;                                               // We have RBP, RDI, and the pointer to the current function stored 
                                                                // and odd number of locals
      } else if (isTop && locNum % 2 == 1) {
        locNum++;
      }

      code->sub_rm_imm(RSP, locNum*VAR_SIZE);

      if (isTop && !noArgs) {
        Scope::VarIterator vi(af->node()->body()->scope());
        
        while (vi.hasNext()) {
          vi.next();
          nargs++;
        }

        code->push_r(RDI);
        code->mov_rr(RSI, RDI);
        code->mov_rr(RDI, RBP);
        code->sub_rm_imm(RDI, (_localsBase + nargs)*VAR_SIZE);
        code->mov_r_imm(RCX, nargs);
        code->add(REPNE);
        code->add(x86_rex(0, 0, 1));
        code->add(MOVS_W);        
      }

      for (NativeFunction::Vars::const_iterator vit = _curf->vars().begin();
           vit != _curf->vars().end();
           ++vit) {
        if (vit->second->_stor == FlowVar::STOR_EXTERN) {
          code->mov_r_imm(RDX, (uint64_t)vit->second->_vi->owner);
          code->mov_rr(RAX, RBP);

          code->mov_rm(RAX, RAX, 0);   // RAX --- the previous stack pointer
          code->mov_rm(RCX, RAX, -8);  // RCX --- the current NativeFunction
          code->cmp_rr(RCX, RDX);
          code->jcc_rel32(CC_NE, -23);  // 6 + 6 + 6 + 5

          code->sub_r_imm(RAX, VAR_SIZE*(_localsBase + vit->second->_vi->fPos)); // it's better to use LEA...
          code->mov_mr(RAX, RBP, -VAR_SIZE*(_extBase + vit->second->_storIdx));
        }
      }

      _code = code;
      generate(info(af->node())->fn);

      if (isTop && !noArgs) {
        code->pop_r(RDI);
        code->mov_rr(RSI, RBP);
        code->sub_rm_imm(RSI, (_localsBase + nargs)*VAR_SIZE);

        code->mov_r_imm(RCX, nargs);
        code->add(REPNE);
        code->add(x86_rex(0, 0, 1));
        code->add(MOVS_W);
      }

      code->add_rm_imm(RSP, (locNum + 1)*VAR_SIZE);  // locNum + owner
      code->pop_r(RBP);

      code->ret(af->parametersNumber()*VAR_SIZE);
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

        case FlowNode::MOD:
          genMod(node);
          break;

        case FlowNode::NOP:
          break;

        case FlowNode::JUMP:
          genJump(node);
          break;

        case FlowNode::CALL:
          genCall(node);
          break;

        case FlowNode::ALIGN:
          _code->sub_r_imm(RSP, 8);
          break;

        case FlowNode::UNALIGN:
          _code->add_r_imm(RSP, 8);
          break;


        case FlowNode::I2D:
        case FlowNode::D2I:
          genCvt(node);
          break;

        default:
          ABORT("Not supported");          
        }
      }
    }

#define INT_OP(op)                                                      \
    switch (dst->_stor) {                                               \
    case FlowVar::STOR_REGISTER:                                        \
      switch (src->_stor) {                                             \
      case FlowVar::STOR_CONST:                                         \
        _code->op(ireg(dst), Imm(src->_const.intConst));                \
        break;                                                          \
                                                                        \
      case FlowVar::STOR_CALL:                                          \
      case FlowVar::STOR_ARG:                                           \
      case FlowVar::STOR_LOCAL:                                         \
        _code->op(ireg(dst), mem(src));                                 \
        break;                                                          \
                                                                        \
      case FlowVar::STOR_EXTERN:                                        \
        _code->mov(Reg(RAX), mem(src));                                 \
        _code->op(ireg(dst), mem(RAX));                                 \
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
    case FlowVar::STOR_CALL:                                            \
    case FlowVar::STOR_ARG:                                             \
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
      case FlowVar::STOR_LOCAL:                                         \
      case FlowVar::STOR_ARG:                                           \
      case FlowVar::STOR_CALL:                                          \
        _code->mov(Reg(RAX), mem(src));                                 \
        _code->op(mem(dst), Reg(RAX));                                  \
        break;                                                          \
                                                                        \
      default:                                                          \
        ABORT("Not supported");                                         \
        }                                                               \
      break;                                                            \
                                                                        \
    case FlowVar::STOR_TEMP:                                            \
    switch (src->_stor) {                                               \
      case FlowVar::STOR_ARG:                                           \
      case FlowVar::STOR_CALL:                                          \
      case FlowVar::STOR_LOCAL:                                         \
        _code->op(Reg(RAX), mem(src));                                  \
        break;                                                          \
                                                                        \
      case FlowVar::STOR_REGISTER:                                      \
        _code->op(Reg(RAX), ireg(src));                                 \
        break;                                                          \
                                                                        \
      case FlowVar::STOR_CONST:                                         \
        _code->op(Reg(RAX), Imm(src->_const.intConst));                 \
        break;                                                          \
                                                                        \
      default:                                                          \
        ABORT("Not supported");                                         \
      }                                                                 \
      break;                                                            \
                                                                        \
    case FlowVar::STOR_EXTERN:                                          \
      switch(src->_stor) {                                              \
      case FlowVar::STOR_REGISTER:                                      \
        _code->mov(Reg(RAX), mem(dst));                                 \
        _code->op(mem(RAX), ireg(src));                                 \
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
      case FlowVar::STOR_CALL:                                          \
      case FlowVar::STOR_ARG:                                           \
      case FlowVar::STOR_LOCAL:                                         \
        _code->op(XmmReg(dst->_storIdx), mem(src));                     \
        break;                                                          \
                                                                        \
      case FlowVar::STOR_REGISTER:                                      \
        _code->op(XmmReg(dst->_storIdx), XmmReg(src->_storIdx));        \
        break;                                                          \
                                                                        \
      case FlowVar::STOR_TEMP:                                          \
        _code->op(Reg(RAX), XmmReg(src->_storIdx));                     \
        break;                                                          \
                                                                        \
      default:                                                          \
        ABORT("Not supported");                                         \
      }                                                                 \
      break;                                                            \
                                                                        \
    case FlowVar::STOR_CALL:                                            \
    case FlowVar::STOR_ARG:                                             \
    case FlowVar::STOR_LOCAL:                                           \
      switch (src->_stor) {                                             \
      case FlowVar::STOR_REGISTER:                                      \
        _code->op(mem(dst), XmmReg(src->_storIdx));                     \
        break;                                                          \
                                                                        \
      case FlowVar::STOR_TEMP:                                          \
        _code->op(mem(dst), Reg(RAX));                                  \
        break;                                                          \
                                                                        \
      default:                                                          \
        ABORT("Not supported");                                         \
      }                                                                 \
      break;                                                            \
                                                                        \
    case FlowVar::STOR_TEMP:                                            \
      switch (src->_stor) {                                             \
      case FlowVar::STOR_CALL:                                          \
      case FlowVar::STOR_ARG:                                           \
      case FlowVar::STOR_LOCAL:                                         \
        _code->op(Reg(RAX), mem(src));                                  \
        break;                                                          \
                                                                        \
      case FlowVar::STOR_REGISTER:                                      \
        _code->op(Reg(RAX), XmmReg(src->_storIdx));                     \
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
      case FlowVar::STOR_CALL:                                          \
      case FlowVar::STOR_ARG:                                           \
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
    case FlowVar::STOR_CALL:                                            \
    case FlowVar::STOR_ARG:                                             \
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

    if (src->_type != VT_STRING) {
      BIN_OP(mov);
    } else {
      // I don't know where to put this code

      switch (dst->_stor) {
      case FlowVar::STOR_TEMP:
        switch (src->_stor) {
        case FlowVar::STOR_CONST:
          _code->mov_r_imm(RAX, (uint64_t)src->_const.stringConst);
          break;

        case FlowVar::STOR_ARG:
        case FlowVar::STOR_LOCAL:
          _code->mov(Reg(RAX), mem(src));
          //          _code->mov(Reg(RAX), mem(src));
          break;

        default:
          ABORT("Not supported");
        }
        break;

      case FlowVar::STOR_LOCAL:
        switch (src->_stor) {
        case FlowVar::STOR_TEMP:
          _code->mov(mem(dst), Reg(RAX));
          break;

        case FlowVar::STOR_CALL:
          _code->mov(Reg(RAX), mem(src));
          _code->mov(mem(dst), Reg(RAX));
          break;
          
        default:
          ABORT("Not supported");
        }               
      break;

      case FlowVar::STOR_CALL:
        switch (src->_stor) {
        case FlowVar::STOR_TEMP:
          _code->mov(mem(dst), Reg(RAX));
          break;

        default:
         ABORT("Not supported");
        }
        break;

      default:
        ABORT("Not supported");
      }
    }
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

  void genMod(FlowNode* node) {
    FlowVar* src = node->u.op.u.bin.op2;
    FlowVar* dst = node->u.op.u.bin.op1;

    BIN_OP(mod);
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
    case VT_STRING:
      switch (op->_stor) {
      case FlowVar::STOR_REGISTER:
        _code->push(ireg(op));
        break;

      case FlowVar::STOR_ARG:  
      case FlowVar::STOR_CALL:
      case FlowVar::STOR_LOCAL:
        _code->push(mem(op));
        break;

      case FlowVar::STOR_TEMP:
        _code->push_r(RAX);
        break;

      case FlowVar::STOR_EXTERN:
        _code->mov(Reg(RAX), mem(op));
        _code->mov(Reg(RAX), mem(RAX));
        _code->push_r(RAX);
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

      case FlowVar::STOR_ARG: 
      case FlowVar::STOR_CALL:
      case FlowVar::STOR_LOCAL:
        _code->push(mem(op));
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
    PrintNode* pn = node->u.print.ref;
    size_t iarg = 1, darg = 0;
    size_t carg = 0;

    for (size_t i = 0; i < pn->operands(); ++i) {
      AstNode* child = pn->operandAt(i);

      switch (info(child)->type) {
      case VAL_STRING:
        if (child->isStringLiteralNode()) {
          break;
        }

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
    case VT_STRING:
    case VT_INT:
      INT_OP(cmp);

      if (node->u.op.trueBranch) {
        cond = posCond[node->type - FlowNode::LT];
      } else {
        cond = negCond[node->type - FlowNode::LT];
      }

      node->label = _code->jcc_rel32(cond);
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

  // --------------------------------------------------------------------------------

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

  // --------------------------------------------------------------------------------

  void genCall(FlowNode* node) {
    FunctionNode* fn = node->u.op.u.call.af->node();
    if (!info(fn)->procAddress) { 
      // A function we compiled

      NativeFunction* callee = info(fn)->funRef;
      
      assert(callee != NULL);
      
      _curf->addRef(_code->call_rel(), callee);    
    } else {                   
      // An external native function

      AstFunction* af = node->u.op.u.call.af;
      size_t pn = af->parametersNumber();
      size_t iarg, darg, carg;

      iarg = darg = carg = 0;

      for (size_t i = 0; i < pn; ++i) {
        switch (af->parameterType(i)) {
        case VAL_STRING:
        case VAL_INT:
          _code->mov_rm(iamap[iarg], RSP, VAR_SIZE*carg);
          iarg++;
          carg++;
          break;

        case VAL_DOUBLE:
          _code->movq_xmm_m(darg, RSP, VAR_SIZE*carg);
          darg++;
          carg++;
          break;

        default:
          break;
        }
      }

      _code->mov_r_imm(RAX, darg);
      _code->add_r_imm(RSP, VAR_SIZE*pn);
      _code->mov_r_imm(RBX, (uint64_t)info(fn)->procAddress);
      _code->call_r(RBX);

      if (af->returnType() == VT_DOUBLE) {
        _code->movq_r_xmm(RAX, XMM0);
      }
    }
  }

  // --------------------------------------------------------------------------------

  void genCvt(FlowNode* node) {
    node->u.op.result->_stor = node->u.op.u.un.op->_stor;
    node->u.op.result->_storIdx = node->u.op.u.un.op->_storIdx;

    switch (node->u.op.result->_type) {
    case VT_INT:
      if (node->u.op.u.un.op->_stor == FlowVar::STOR_REGISTER) {
        _code->cvtsd2si(ireg(node->u.op.result), XmmReg(node->u.op.u.un.op->_storIdx));
      } else {
        ABORT("Not supported");
      }

      break;

    case VT_DOUBLE:
      if (node->u.op.u.un.op->_stor == FlowVar::STOR_REGISTER) {
        _code->cvtsi2sd(XmmReg(node->u.op.result->_storIdx), ireg(node->u.op.u.un.op));
      } else {
        ABORT("Not supported");
      }
      break;

    default:
      ABORT("Not supported");
    }
  }

  // --------------------------------------------------------------------------------

  Mem mem(FlowVar* fv) {
    switch (fv->_stor) {
    case FlowVar::STOR_LOCAL:
      return Mem(RBP, -VAR_SIZE*(_localsBase + fv->_vi->fPos));
      break;

    case FlowVar::STOR_CALL:
      return Mem(RBP, -VAR_SIZE*(_callBase + fv->_vi->fPos));
      break;

    case FlowVar::STOR_ARG:
      return Mem(RBP, VAR_SIZE*fv->_vi->fPos + 16);
      break;

    case FlowVar::STOR_EXTERN:
      return Mem(RBP, -VAR_SIZE*(_extBase + fv->_storIdx));
      break;

    default:
      ABORT("Not supported yet");
    }
  }

  Mem mem(int base) {
    return Mem(base, 0);
  }

  Reg ireg(FlowVar* fv) {
    return Reg(rmap[fv->_storIdx]);
  }
    
  private:
    X86Code* _code;
    //AstFunction* _curf;
    NativeFunction* _curf;

    size_t _localsBase;
    size_t _extBase;
    size_t _callBase;
  };
}
