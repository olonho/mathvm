#pragma once

#include "common.h"
#include "Runtime.h"

// ================================================================================

namespace mathvm {
  #define VAR_SIZE 8

  class NativeGenerator2 {
    static char rmap[] = {RAX, RCX, RDX, RBX, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15};

  public:
    NativeGenerator2(AstFunction* af) {
      FlowNode* node = info(af->node())->fn;
      X86Code* code = info(af->node())->funRef;
      size_t nargs = 0;

      code->push_r(RBP);
      code->mov_rr(RBP, RSP);
      code->sub_rm_imm(RSP, info(af->node())->funRef->localsNumber()*VAR_SIZE);

      if (af->name() == AstFunction::top_name) {
        Scope::VarIterator vi(af->node()->body()->scope());
        
        while (vi.hasNext()) {
          vi.next();
          nargs++;
        }

        code->mov_rr(RDI, RBP);
        code->sub_rm_imm(RDI, nargs*VAR_SIZE);
        code->mov_r_imm(RCX, nargs);
        code->add(REPNE);
        code->add(x86_rex(0, 0, 1));
        code->add(MOVS_W);
      }

      _code = code;
      generate();

      if (af->name() == AstFunction::top_name) {
      }

      code->add_rm_imm(RSP, info(af->node())->funRef->localsNumber()*VAR_SIZE);
      code->pop_r(RBP);
      code->add(RET);      
    }

    
    void generate() {
      for (; node; node = node->next) {
        switch (node->type) {
        case FlowNode::ASSIGN:
          switch (node->u.op.u.un.op->stor) {
          case FlowVar::STOR_REGISTER:
            _code->mov_mr(rmap[node->u.op.u.un.op->storIdx], RBP, -VAR_SIZE*info(node->var())->fPos - 8);
            break;

          default:
            ABORT("Not supported");
          }
          break;

        default:
          break;          
        }
      }
    }

  private:
    X86Code* _code;
  }
}
